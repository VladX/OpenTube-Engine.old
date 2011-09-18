/*
 * This file is part of Opentube - Open video hosting engine
 *
 * Copyright (C) 2011 - VladX; http://vladx.net/
 *
 * Opentube is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Opentube is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Opentube; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include "common_functions.h"

#ifdef _WIN

#include "core_server.h"
#include "win32_utils.h"
#include <windows.h>
#include <lm.h>
#include <ntsecapi.h>

bool win32_service_running = false;
SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hStatus;

void ControlHandler (DWORD request)
{
	switch (request)
	{
		case SERVICE_CONTROL_STOP:
			ServiceStatus.dwWin32ExitCode = 0;
			ServiceStatus.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(hStatus, &ServiceStatus);
			quit(0);
			
			return;
		
		case SERVICE_CONTROL_SHUTDOWN:
			ServiceStatus.dwWin32ExitCode = 0;
			ServiceStatus.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(hStatus, &ServiceStatus);
			quit(0);
			
			return;
		
		default:
			break;
	}
	
	SetServiceStatus(hStatus, &ServiceStatus);
}

void ServiceMain (int argc, char ** argv)
{
	WCHAR * userprofile = NULL;
	uint userprofile_size;
	DWORD ret = 256;
	
	do
	{
		userprofile_size = ret;
		userprofile = (WCHAR *) allocator_realloc(userprofile, userprofile_size * sizeof(WCHAR));
		ret = GetEnvironmentVariableW(L"USERPROFILE", userprofile, userprofile_size);
		if (ret == 0)
			break;
	}
	while (ret > userprofile_size);
	
	if (ret != 0)
		if (SetCurrentDirectoryW(userprofile) == 0)
			win32_fatal_error("SetCurrentDirectoryW()");
	
	allocator_free(userprofile);
	
	userprofile = NULL;
	
	logger_set_file_output();
	
	ServiceStatus.dwServiceType = SERVICE_WIN32;
	ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	ServiceStatus.dwWin32ExitCode = NO_ERROR;
	ServiceStatus.dwServiceSpecificExitCode = 0;
	ServiceStatus.dwCheckPoint = 0;
	ServiceStatus.dwWaitHint = 0;
	
	hStatus = RegisterServiceCtrlHandler(SHORT_PROG_NAME, (LPHANDLER_FUNCTION) ControlHandler);
	
	if (hStatus == (SERVICE_STATUS_HANDLE) 0)
		return;
	
	ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(hStatus, &ServiceStatus);
	
	win32_service_running = true;
	
	while (ServiceStatus.dwCurrentState == SERVICE_RUNNING)
	{
		debug_print_1("Windows service started successfully (current state = %d)", (int) SERVICE_RUNNING);
		init(* argv);
	}
}

static void set_failure_actions (SC_HANDLE hService)
{
	SERVICE_FAILURE_ACTIONS sfa;
	SC_ACTION actions[1];
	
	sfa.dwResetPeriod = 60 * 60 * 24;
	sfa.lpRebootMsg = NULL;
	sfa.lpCommand = NULL;
	sfa.cActions = ARRAY_LENGTH(actions);
	sfa.lpsaActions = actions;
	
	actions[0].Type = SC_ACTION_RESTART;
	actions[0].Delay = 0;
	
	if (!ChangeServiceConfig2(hService, SERVICE_CONFIG_FAILURE_ACTIONS, &sfa))
		win32_fatal_error("ChangeServiceConfig2()");
}

static void service_try_start (SC_HANDLE hService)
{
	if (StartService(hService, 0, NULL) == 0)
		perr("%s", "starting service");
}

static bool service_try_install (void)
{
	char bin_path[MAX_PATH];
	char conf_path[MAX_PATH];
	char * full_path;
	SC_HANDLE hSCManager, hService;
	char * lpServiceStartName;
	extern const char * path_to_configuration_file;
	
	if(!GetModuleFileNameA(NULL, bin_path, MAX_PATH))
		win32_fatal_error("GetModuleFileName()");
	
	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	
	if (hSCManager == NULL)
	{
		if (GetLastError() == ERROR_ACCESS_DENIED)
			return false;
		
		win32_fatal_error("OpenSCManager()");
	}
	
	lpServiceStartName = allocator_malloc(strlen(config.group) + strlen(config.user) + 2);
	strcpy(lpServiceStartName, ".\\");
	strcat(lpServiceStartName, config.user);
	
	if (GetFullPathNameA(path_to_configuration_file, MAX_PATH, conf_path, NULL) == 0)
		peerr(-1, "GetFullPathNameA(%s)", path_to_configuration_file);
	
	full_path = allocator_malloc(strlen(bin_path) + strlen(conf_path) + 9);
	strcpy(full_path, "\"");
	strcat(full_path, bin_path);
	strcat(full_path, "\" -c \"");
	strcat(full_path, conf_path);
	strcat(full_path, "\"");
	
	hService = CreateServiceA(hSCManager, SHORT_PROG_NAME, PROG_NAME, SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, full_path, NULL, NULL, NULL, lpServiceStartName, WIN32_DEFAULT_USER_PASWORD);
	
	allocator_free(lpServiceStartName);
	allocator_free(full_path);
	
	if (hService == NULL)
		switch (GetLastError())
		{
			case ERROR_SERVICE_EXISTS:
			case ERROR_DUPLICATE_SERVICE_NAME:
				return false;
			default:
				win32_fatal_error("CreateServiceA()");
		}
	
	set_failure_actions(hService);
	service_try_start(hService);
	
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	
	return true;
}

static void service_change_conf (void)
{
	SC_HANDLE hSCManager, hService;
	char * lpServiceStartName;
	
	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	
	if (hSCManager == NULL)
	{
		if (GetLastError() == ERROR_ACCESS_DENIED)
			return;
		
		win32_fatal_error("OpenSCManager()");
	}
	
	hService = OpenServiceA(hSCManager, SHORT_PROG_NAME, SERVICE_CHANGE_CONFIG | SERVICE_START);
	
	if (hService == NULL)
		win32_fatal_error("OpenServiceA()");
	
	lpServiceStartName = allocator_malloc(strlen(config.group) + strlen(config.user) + 2);
	
	strcpy(lpServiceStartName, ".\\");
	strcat(lpServiceStartName, config.user);
	
	if (!ChangeServiceConfigA(hService, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, NULL, NULL, NULL, NULL, lpServiceStartName, WIN32_DEFAULT_USER_PASWORD, NULL))
		win32_fatal_error("ChangeServiceConfigA()");
	
	allocator_free(lpServiceStartName);
	
	set_failure_actions(hService);
	service_try_start(hService);
	
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
}

static void add_service_user (void)
{
	NET_API_STATUS nStatus;
	USER_INFO_1 ui;
	LOCALGROUP_INFO_0 gi;
	LOCALGROUP_MEMBERS_INFO_3 gmi;
	
	assert(strlen(config.user) > 0);
	
	if (_stricmp(config.user, config.group) == 0)
		eerr(-1, "%s", "The name of the user and group can not be the same on Windows.");
	
	gi.lgrpi0_name = win32_utf8_to_utf16(config.group);
	
	nStatus = NetLocalGroupAdd(NULL, 0, (LPBYTE) &gi, NULL);
	
	if (nStatus == ERROR_ACCESS_DENIED)
	{
		allocator_free(gi.lgrpi0_name);
		
		return;
	}
	
	if (nStatus != NERR_Success && nStatus != ERROR_ALIAS_EXISTS)
		eerr(-1, "NetLocalGroupAdd(): %d", (int) nStatus);
	
	ui.usri1_name = win32_utf8_to_utf16(config.user);
	ui.usri1_password = win32_utf8_to_utf16(WIN32_DEFAULT_USER_PASWORD);
	ui.usri1_priv = USER_PRIV_USER;
	ui.usri1_home_dir = win32_utf8_to_utf16((char *) config.document_root.str);
	ui.usri1_comment = NULL;
	ui.usri1_flags = UF_SCRIPT | UF_DONT_EXPIRE_PASSWD;
	ui.usri1_script_path = NULL;
	
	nStatus = NetUserAdd(NULL, 1, (LPBYTE) &ui, NULL);
	
	if (nStatus != NERR_Success && nStatus != NERR_UserExists && nStatus != NERR_GroupExists)
		eerr(-1, "NetUserAdd(): %d", (int) nStatus);
	
	gmi.lgrmi3_domainandname = ui.usri1_name;
	
	nStatus = NetLocalGroupAddMembers(NULL, gi.lgrpi0_name, 3, (LPBYTE) &gmi, 1);
	
	if (nStatus != NERR_Success && nStatus != ERROR_MEMBER_IN_ALIAS)
		eerr(-1, "NetLocalGroupAddMembers(): %d", (int) nStatus);
	
	allocator_free(gi.lgrpi0_name);
	allocator_free(ui.usri1_name);
	allocator_free(ui.usri1_password);
	allocator_free(ui.usri1_home_dir);
	
	_BEGIN_LOCAL_SECTION_
	NTSTATUS ret;
	LSA_OBJECT_ATTRIBUTES dummystruct;
	LSA_HANDLE hPolicy;
	DWORD cbsid = 256;
	DWORD rdnsz = 256;
	PSID sid = NULL;
	LPSTR ReferencedDomainName = NULL;
	SID_NAME_USE peUse;
	
	memset(&dummystruct, 0, sizeof(dummystruct));
	
	ret = LsaOpenPolicy(NULL, &dummystruct, POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES, &hPolicy);
	
	if (ret != 0)
		eerr(-1, "LsaOpenPolicy(): %d", (int) LsaNtStatusToWinError(ret));
	
	again:
	
	sid = (PSID) allocator_realloc(sid, cbsid);
	ReferencedDomainName = (LPSTR) allocator_realloc(ReferencedDomainName, rdnsz);
	
	if (!LookupAccountNameA(NULL, config.user, sid, &cbsid, ReferencedDomainName, &rdnsz, &peUse))
	{
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			cbsid *= 2;
			rdnsz *= 2;
			goto again;
		}
		
		if (GetLastError() == ERROR_ACCESS_DENIED)
			return;
		
		win32_fatal_error("LookupAccountNameA()");
	}
	
	_BEGIN_LOCAL_SECTION_
	LSA_UNICODE_STRING UserRights[1];
	
	WCHAR * se_service_logon_name = L"SeServiceLogonRight";
	USHORT len = (USHORT) wcslen(se_service_logon_name);
	UserRights[0].Buffer = se_service_logon_name;
	UserRights[0].Length = (USHORT) len * sizeof(WCHAR);
	UserRights[0].MaximumLength = (USHORT) (len + 1) * sizeof(WCHAR);
	
	ret = LsaAddAccountRights(hPolicy, sid, UserRights, 1);
	
	if (ret != 0)
		eerr(-1, "LsaAddAccountRights(): %d", (int) LsaNtStatusToWinError(ret));
	
	LsaClose(hPolicy);
	allocator_free(sid);
	allocator_free(ReferencedDomainName);
	
	_END_LOCAL_SECTION_
	_END_LOCAL_SECTION_
}

void win32_service_init (void)
{
	SERVICE_TABLE_ENTRYA ServiceTable[2];
	
	add_service_user();
	
	if (!service_try_install())
		service_change_conf();
	
	ServiceTable[0].lpServiceName = SHORT_PROG_NAME;
	ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTION) ServiceMain;
	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;
	
	if (StartServiceCtrlDispatcherA(ServiceTable) == 0)
		switch (GetLastError())
		{
			case ERROR_FAILED_SERVICE_CONTROLLER_CONNECT:
				#if DEBUG_LEVEL
				return;
				#else
				break;
				#endif
			case ERROR_INVALID_DATA:
				eerr(-1, "Invalid data passed to function StartServiceCtrlDispatcherA() (%d)", (int) ERROR_INVALID_DATA);
			case ERROR_SERVICE_ALREADY_RUNNING:
				eerr(-1, "%s", "Service is already running.");
			default:
				win32_fatal_error("StartServiceCtrlDispatcherA()");
		}
	
	exit(0);
}
#endif
