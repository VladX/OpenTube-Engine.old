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
#include <QRegExp>

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
static unsigned int try_num = 0;

bool get_files_index (QObject * obj, QBuffer * buf)
{
	unsigned int i = try_num;
	
	if (i >= (sizeof(hosts) / sizeof(hosts[0])))
		return false;
	
	if (i == 0)
		obj->connect(&http, SIGNAL(done(bool)), SLOT(indexDownloaded(bool)));
	
	http.setHost(hosts[i].host, hosts[i].mode);
	path = hosts[i].start_path;
	path.append((sizeof(void *) == 4) ? "win32/" : "win64/");
	QString str = path;
	str.append("index.md5");
	http.get(str, buf);
	try_num++;
	
	return true;
}

bool parse_files_index (QBuffer * buf)
{
	int offset = 0;
	QRegExp re("([0-9a-f]{32})\\s+([\\w/-][\\w/ -]*)", Qt::CaseInsensitive);
	
	while ((offset = re.indexIn(buf->data().constData(), offset) + 1) > 0)
	{
		index_hashes.append(re.capturedTexts()[1]);
		index_files.append(re.capturedTexts()[2]);
	}
	
	return (index_hashes.size() > 0);
}
