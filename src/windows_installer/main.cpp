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

#include "wizard.h"
#include "utils.h"
#include "win7_taskbar.h"
#include "install_archive.h"
#include "../config.h"

bool local_installation = false;
QString ar_data_root;

#ifdef WIN_TASKBAR_STAFF
DEFINE_GUID(CLSID_TaskbarList, 0x56fdf344, 0xfd6d, 0x11d0, 0x95, 0x8a, 0x00, 0x60, 0x97, 0xc9, 0xa0, 0x90);
DEFINE_GUID(IID_ITaskbarList3, 0xea1afb91, 0x9e28, 0x4b86, 0x90, 0xE9, 0x9e, 0x9f, 0x8a, 0x5e, 0xef, 0xaf);
#endif

QOPSetupWizard::QOPSetupWizard (QWidget * parent) : QWizard(parent)
{
	this->setCancelable(true);
#ifdef WIN_TASKBAR_STAFF
	this->mWindowId = this->winId();
	this->mTaskbar = NULL;
	this->mTaskbarMessageId = RegisterWindowMessageA("TaskbarButtonCreated");
#endif
}

#ifdef WIN_TASKBAR_STAFF
bool QOPSetupWizard::winEvent (MSG * message, long * result)
{
	if (message->message == this->mTaskbarMessageId)
	{
		HRESULT hr = CoCreateInstance(CLSID_TaskbarList, 0, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, reinterpret_cast<void**> (&(this->mTaskbar)));
		* result = hr;
		
		if (hr != S_OK)
			this->mTaskbar = NULL;
		
		return true;
	}
	
	return false;
}
#endif

void QOPSetupWizard::setProgressValue (int value, int max)
{
#ifdef WIN_TASKBAR_STAFF
	if (this->mTaskbar == NULL)
		return;
	
	this->mTaskbar->SetProgressValue(this->mWindowId, value, max);
#endif
}

void QOPSetupWizard::setProgressState (TBPFLAG state)
{
#ifdef WIN_TASKBAR_STAFF
	if (this->mTaskbar == NULL)
		return;
	
	this->mTaskbar->SetProgressState(this->mWindowId, state);
#endif
}

static QString unpack_archive (const QString & prefix, const QString & cur_dir)
{
	InstallArchive ar(cur_dir);
	
	if (!ar.exists())
		return NULL;
	
	QString tempdir = prefix;
	
	if (tempdir.length() == 0)
		tempdir = "installer";
	
	tempdir.append("_data");
	QDir temp = QDir::temp();
	QString arch_root = temp.absoluteFilePath(tempdir);
	recursive_remove_dir(arch_root);
	temp.mkdir(tempdir);
	QDir arch_root_dir(arch_root);
	
	if (!arch_root_dir.exists(arch_root))
		return NULL;
	
	if (!ar.extract(arch_root))
		return NULL;
	
	return arch_root;
}

int main (int argc, char ** argv)
{
	QApplication app(argc, argv, true);
	QOPSetupWizard wizard;
	ar_data_root = unpack_archive(app.applicationName(), app.applicationDirPath());
	
	local_installation = ar_data_root.length() > 0;
	
	wizard.setWindowTitle(PROG_NAME " Setup");
	setup_wizard(wizard);
	wizard.show();
	
	int code = app.exec();
	
	if (local_installation)
		recursive_remove_dir(ar_data_root);
	
	remove_temp_dir();
	
	return code;
}
