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

#include <QDir>
#include "install_archive.h"

const QString InstallArchive::archiveName(ARCHIVE_NAME);
static InstallArchive * this_ptr;

static int openfunc (const char * path, int flags, ...)
{
	if (this_ptr->open(QIODevice::ReadOnly))
		return 0;
	
	return -1;
}

static int closefunc (int fd)
{
	this_ptr->close();
	
	return 0;
}

static ssize_t readfunc (int fd, void * data, size_t size)
{
	return (ssize_t) this_ptr->read(reinterpret_cast<char *>(data), (qint64) size);
}

static ssize_t writefunc (int fd, const void * data, size_t size)
{
	return (ssize_t) this_ptr->write(reinterpret_cast<const char *>(data), (qint64) size);
}

InstallArchive::InstallArchive (void) : QFile(InstallArchive::archiveName) {}

InstallArchive::InstallArchive (const QString & dir)
{
	QDir d(dir);
	this->setFileName(d.absoluteFilePath(InstallArchive::archiveName));
}

bool InstallArchive::extract (QString & dir)
{
	if (!this->exists())
		return false;
	
	TAR * t;
	tartype_t ttype;
	ttype.openfunc = reinterpret_cast<openfunc_t>(openfunc);
	ttype.closefunc = reinterpret_cast<closefunc_t>(closefunc);
	ttype.readfunc = reinterpret_cast<readfunc_t>(readfunc);
	ttype.writefunc = reinterpret_cast<writefunc_t>(writefunc);
	this_ptr = this;
	
	if (tar_open(&t, NULL, &ttype, 0, 0, 0) == -1)
		return false;
	
	if (tar_extract_all(t, dir.toAscii().data()) == -1)
		return false;
	
	return true;
}
