/*
 * This file is part of Opentube - Open video hosting engine
 *
 * Copyright (C) 2011 - Xpast; http://xpast.me/; <vvladxx@gmail.com>
 *
 * Opentube is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Opentube is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Opentube.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "get_from_remote_site.h"
#include "utils.h"
#include <QRegExp>
#include <QDir>

#define DOWNLOAD_BRANCH_NAME "binary-stable"

static const char * temp_dir_name = "installer_dl_data";

struct r_host
{
	const char * host;
	const char * start_path;
	const char * trailer_path;
	QHttp::ConnectionMode mode;
};

static const struct r_host hosts[] = {
	{
		"raw.github.com",
		"/VladX/OpenTube-Engine/" DOWNLOAD_BRANCH_NAME "/",
		"",
		QHttp::ConnectionModeHttps
	},
	{
		"opentube-engine.googlecode.com",
		"/git/",
		"?r=" DOWNLOAD_BRANCH_NAME,
		QHttp::ConnectionModeHttp
	},
	{
		"opentube-engine.git.sourceforge.net",
		"/git/gitweb.cgi?p=opentube-engine/opentube-engine;a=blob_plain;hb=" DOWNLOAD_BRANCH_NAME ";f=",
		"",
		QHttp::ConnectionModeHttp
	}
};

static QHttp http;
static QString path;
static QString trailer_path;
static QStringList index_hashes;
static QStringList index_files;
static QDir * temp_dir = NULL;
static QFile * files = NULL;
static unsigned int try_num = 0;


QString get_dl_temp_dir (void)
{
	if (temp_dir == NULL)
		return NULL;
	
	return temp_dir->absolutePath();
}

bool get_files_index (QObject * obj, QBuffer * buf)
{
	unsigned int i = try_num;
	
	if (i >= (sizeof(hosts) / sizeof(hosts[0])))
		return false;
	
	if (i == 0)
		obj->connect(&http, SIGNAL(done(bool)), SLOT(indexDownloaded(bool)));
	
	http.connect(&http, SIGNAL(sslErrors(const QList<QSslError> &)), SLOT(ignoreSslErrors()));
	http.setHost(hosts[i].host, hosts[i].mode);
	path = hosts[i].start_path;
	trailer_path = hosts[i].trailer_path;
	path.append((os_is64bit()) ? "win64/" : "win32/");
	QString str = path;
	str.append("index.md5");
	str.append(trailer_path);
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
	http.connect(&http, SIGNAL(sslErrors(const QList<QSslError> &)), SLOT(ignoreSslErrors()));
	obj->connect(&http, SIGNAL(requestFinished(int, bool)), SLOT(fileDownloaded(int, bool)));
	obj->connect(&http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)), SLOT(responseHeaderReceived(const QHttpResponseHeader &)));
	obj->connect(&http, SIGNAL(dataReadProgress(int, int)), SLOT(dataReadProgress(int, int)));
	
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
		str.append(trailer_path);
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
