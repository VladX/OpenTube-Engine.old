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

#include "detect_os.h"
#include <QDir>
#include <QFile>

#ifdef OS_WINDOWS
#include <windows.h>
#endif

void recursive_remove_dir (const QString & dir)
{
	QDir d(dir);
	
	if (!d.exists())
		return;
	
	int i;
	QFileInfoList l = d.entryInfoList();
	QFileInfo f;
	QString path;
	
	for (i = 0; i < l.count(); i++)
	{
		f = l[i];
		
		if (f.fileName() == "." || f.fileName() == "..")
			continue;
		
		if (f.isDir())
			recursive_remove_dir(f.absoluteFilePath());
		else
			d.remove(f.fileName());
	}
	
	path = d.dirName();
	d.cdUp();
	d.rmdir(path);
}

void recursive_copy_dir (const QString & src_dir, const QString & dst_dir)
{
	QDir s(src_dir);
	QDir d(dst_dir);
	
	if (!s.exists())
		return;
	
	recursive_remove_dir(dst_dir);
	
	if (!d.exists())
	{
		QString dirname = d.dirName();
		d.cdUp();
		d.mkpath(dirname);
		d.cd(dirname);
	}
	
	int i;
	QFileInfoList l = s.entryInfoList();
	QFileInfo f;
	
	for (i = 0; i < l.count(); i++)
	{
		f = l[i];
		
		if (f.fileName() == "." || f.fileName() == "..")
			continue;
		
		if (f.isDir())
			recursive_copy_dir(f.absoluteFilePath(), d.absoluteFilePath(f.fileName()));
		else
			QFile(f.absoluteFilePath()).copy(d.absoluteFilePath(f.fileName()));
	}
}

void try_remove_windows_service (const char * service_name)
{
	#ifdef OS_WINDOWS
	SC_HANDLE hSCManager, hService;
	SERVICE_STATUS_PROCESS ssp;
	
	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	
	if (hSCManager == NULL)
		return;
	
	hService = OpenServiceA(hSCManager, service_name, SERVICE_STOP | DELETE);
	
	if (hService == NULL)
		return;
	
	ControlService(hService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS) &ssp);
	DeleteService(hService);
	
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCManager);
	#endif
}
