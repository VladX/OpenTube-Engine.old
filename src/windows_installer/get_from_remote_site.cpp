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

#include "get_from_remote_site.h"
#include "detect_os.h"
#include "utils.h"
#include <QRegExp>
#include <QDir>

static const char * temp_dir_name = "installer_dl_data";

struct r_host
{
	const char * host;
	const char * start_path;
	QHttp::ConnectionMode mode;
};

static const struct r_host hosts[] = {
	{
		"raw.github.com",
		"/VladX/OpenTube-Engine/binary-stable/",
		QHttp::ConnectionModeHttps
	},
	{
		"opentube-engine.git.sourceforge.net",
		"/git/gitweb.cgi?p=opentube-engine/opentube-engine;a=blob_plain;hb=binary-stable;f=",
		QHttp::ConnectionModeHttp
	}
};

static QHttp http;
static QString path;
static QStringList index_hashes;
static QStringList index_files;
static QDir * temp_dir = NULL;
static QFile * files = NULL;
static unsigned int try_num = 0;

#ifdef OS_WINDOWS
typedef BOOL (WINAPI * LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
#endif

static bool is64bit (void)
{
	if (sizeof(void *) == 4)
	{
		#ifdef OS_WINDOWS
		LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(GetModuleHandleA("kernel32"), "IsWow64Process");
		
		if (fnIsWow64Process == NULL)
			return false;
		
		BOOL is64 = 0;
		
		if (fnIsWow64Process(GetCurrentProcess(), &is64) == 0)
			return false;
		
		return ((is64) ? true : false);
		#else
		return false;
		#endif
	}
	else
		return true;
}

bool get_files_index (QObject * obj, QBuffer * buf)
{
	unsigned int i = try_num;
	
	if (i >= (sizeof(hosts) / sizeof(hosts[0])))
		return false;
	
	if (i == 0)
		obj->connect(&http, SIGNAL(done(bool)), SLOT(indexDownloaded(bool)));
	
	http.setHost(hosts[i].host, hosts[i].mode);
	path = hosts[i].start_path;
	path.append((is64bit()) ? "win64/" : "win32/");
	QString str = path;
	str.append("index.md5");
	http.get(str, buf);
	try_num++;
	
	return true;
}

bool parse_files_index (QBuffer * buf)
{
	int offset = 0;
	QRegExp re("([0-9a-f]{32})\\s+([\\w/\\.-][\\w/\\. -]*)", Qt::CaseInsensitive);
	
	while ((offset = re.indexIn(buf->data().constData(), offset) + 1) > 0)
	{
		index_hashes.append(re.capturedTexts()[1]);
		index_files.append(re.capturedTexts()[2]);
	}
	
	return (index_hashes.size() > 0);
}

int download_all_files (QObject * obj)
{
	if (try_num == 0)
		return 0;
	
	http.disconnect();
	obj->connect(&http, SIGNAL(done(bool)), SLOT(fileDownloaded(bool)));
	obj->connect(&http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)), SLOT(responseHeaderReceived(const QHttpResponseHeader &)));
	
	if (temp_dir == NULL)
		temp_dir = new QDir;
	(* temp_dir) = QDir::temp();
	recursive_remove_dir(temp_dir->absoluteFilePath(temp_dir_name));
	temp_dir->mkdir(temp_dir_name);
	temp_dir->cd(temp_dir_name);
	
	if (files == NULL)
		files = new QFile[index_files.size()];
	
	int i, pos;
	QString str;
	
	for (i = 0; i < index_files.size(); i++)
	{
		str = path;
		str.append(index_files[i]);
		pos = index_files[i].lastIndexOf('/');
		if (pos > 0)
			temp_dir->mkpath(index_files[i].left(pos));
		files[i].setFileName(temp_dir->absoluteFilePath(index_files[i]));
		files[i].open(QIODevice::WriteOnly);
		http.get(str, &(files[i]));
	}
	
	return index_files.size();
}

void finish_downloading (void)
{
	http.disconnect();
	http.close();
	
	int i;
	
	for (i = 0; i < index_files.size(); i++)
		files[i].close();
	
	delete[] files;
	files = NULL;
}

void remove_temp_dir (void)
{
	if (files == NULL)
		return;
	
	int i;
	
	for (i = 0; i < index_files.size(); i++)
		if (files[i].exists())
			files[i].remove();
	
	if (temp_dir == NULL)
		return;
	
	(* temp_dir) = QDir::temp();
	temp_dir->rmdir(temp_dir_name);
}
