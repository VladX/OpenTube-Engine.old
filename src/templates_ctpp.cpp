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

#include <pthread.h>
#include <sys/stat.h>
#include <map>
#include <string>
#include <CTPP2OutputCollector.hpp>
#include <CTPP2StringOutputCollector.hpp>
#include <CTPP2SyscallFactory.hpp>
#include <CTPP2VMSTDLib.hpp>
#include <CTPP2Parser.hpp>
#include <CTPP2FileSourceLoader.hpp>
#include <CTPP2ParserException.hpp>
#include <CTPP2HashTable.hpp>
#include <CTPP2VM.hpp>
#include <CTPP2VMDumper.hpp>
#include <CTPP2VMOpcodes.h>
#include <CTPP2VMMemoryCore.hpp>
#include <CTPP2FileLogger.hpp>

using namespace CTPP;

extern "C"
{

#include "common_functions.h"
#include "win32_utils.h"

#ifdef _WIN
#include <io.h>
#endif

typedef struct
{
	const VMExecutable * exec;
	const VMMemoryCore * vmmemcore;
	std::string data;
	u_str_t * out;
	time_t mtime;
	UINT_32 size;
} compiled_template;

typedef std::map <std::string, compiled_template *> ct_map;

extern char * cur_template_dir;

static threadsafe SyscallFactory * oSyscallFactory;
static threadsafe VM * oVM;
static threadsafe FileLogger * oLogger;
static threadsafe CDT * oHash;

static ct_map compiled_templates;
static pthread_spinlock_t spin_access[1];

#undef error_in_file
#define error_in_file(PATTERN, ...) { err("An error occurred while trying to parse \"%s\": " PATTERN, file, __VA_ARGS__); return false; }


void ctpp_set_var (const char * name, const char * value)
{
	(* oHash)[name] = value;
}

void ctpp_run (const char * file, u_str_t ** out)
{
	pthread_spin_lock(spin_access);
	
	static ct_map::iterator it;
	static std::string mkey;
	
	mkey = file;
	it = compiled_templates.find(mkey);
	
	if (it == compiled_templates.end())
	{
		pthread_spin_unlock(spin_access);
		* out = NULL;
		return;
	}
	
	compiled_template * ct;
	ct = (* it).second;
	
	pthread_spin_unlock(spin_access);
	
	ct->data.clear();
	StringOutputCollector oOutputCollector(ct->data);
	
	oVM->Init(ct->vmmemcore, &oOutputCollector, oLogger);
	UINT_32 iIP = 0;
	oVM->Run(ct->vmmemcore, &oOutputCollector, iIP, * oHash, oLogger);
	
	ct->out->len = ct->data.size();
	ct->out->str = (uchar *) ct->data.data();
	
	* out = ct->out;
}

bool ctpp_compile (const char * file)
{
	pthread_spin_lock(spin_access);
	
	static struct stat st;
	
	int r;
	char * cwd = gnu_getcwd();
	
	if (cwd != NULL)
	{
		r = chdir(cur_template_dir);
		if (r == -1)
		{
			perr("chdir(%s)", cur_template_dir);
			pthread_spin_unlock(spin_access);
			return false;
		}
	}
	
	if (stat(file, &st) == -1)
	{
		perr("stat(%s)", file);
		pthread_spin_unlock(spin_access);
		return false;
	}
	
	if (cwd != NULL)
	{
		r = chdir(cwd);
		if (r == -1)
			peerr(0, "chdir(%s)", cwd);
		free(cwd);
	}
	
	std::string mkey = file;
	
	static ct_map::iterator it;
	it = compiled_templates.find(mkey);
	if (it != compiled_templates.end())
	{
		if (st.st_mtime == ((* it).second)->mtime)
		{
			pthread_spin_unlock(spin_access);
			return true;
		}
		((* it).second)->data.clear();
		delete ((* it).second)->vmmemcore;
		free((void *) ((* it).second)->exec);
		delete ((* it).second)->out;
		delete (* it).second;
	}
	
	VMOpcodeCollector oVMOpcodeCollector;
	StaticText oSyscalls;
	StaticData oStaticData;
	StaticText oStaticText;
	HashTable oHashTable;
	
	CTPP2Compiler oCompiler(oVMOpcodeCollector, oSyscalls, oStaticData, oStaticText, oHashTable);
	
	try
	{
		CTPP2FileSourceLoader oSourceLoader;
		
		std::string includedir = cur_template_dir;
		const std::vector <std::string> includedirs(&includedir, &includedir + 1);
		oSourceLoader.SetIncludeDirs(includedirs);
		oSourceLoader.LoadTemplate(file);
		
		CTPP2Parser oCTPP2Parser(&oSourceLoader, &oCompiler, file);
		oCTPP2Parser.Compile();
	}
	catch (CTPPParserSyntaxError & e)
	{
		pthread_spin_unlock(spin_access);
		error_in_file("At line %u, pos. %u: %s", (uint) e.GetLine(), (uint) e.GetLinePos(), e.what());
	}
	catch (CTPPParserOperatorsMismatch & e)
	{
		pthread_spin_unlock(spin_access);
		error_in_file("At line %u, pos. %u: expected %s, but found </%s>", (uint) e.GetLine(), (uint) e.GetLinePos(), e.Expected(), e.Found());
	}
	catch (CTPPLogicError & e)
	{
		pthread_spin_unlock(spin_access);
		error_in_file("%s", e.what());
	}
	catch (CTPPUnixException & e)
	{
		pthread_spin_unlock(spin_access);
		error_in_file("I/O in %s: %s", e.what(), strerror(e.ErrNo()));
	}
	catch (...)
	{
		pthread_spin_unlock(spin_access);
		error_in_file("%s", "Unknown error");
	}
	
	UINT_32 iCodeSize = 0;
	const VMInstruction * oVMInstruction = oVMOpcodeCollector.GetCode(iCodeSize);
	VMDumper oDumper(iCodeSize, oVMInstruction, oSyscalls, oStaticData, oStaticText, oHashTable);
	UINT_32 iSize = 0;
	const VMExecutable * aProgramCore = oDumper.GetExecutable(iSize);
	compiled_template * ct = new compiled_template;
	ct->exec = (const VMExecutable *) malloc(iSize);
	memcpy((void*) ct->exec, aProgramCore, iSize);
	ct->size = iSize;
	ct->vmmemcore = new VMMemoryCore(ct->exec);
	ct->out = new u_str_t;
	ct->mtime = st.st_mtime;
	compiled_templates[mkey] = ct;
	pthread_spin_unlock(spin_access);
	
	return true;
}

void ctpp_init (void)
{
	pthread_spin_init(spin_access, PTHREAD_PROCESS_PRIVATE);
	oSyscallFactory = new SyscallFactory(config.worker_threads + 1);
	STDLibInitializer::InitLibrary(* (oSyscallFactory));
	oVM = new VM(oSyscallFactory);
	oLogger = new FileLogger(stderr);
	oHash = new CDT();
}

}
