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
#include <QFile>

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
		{
			path = f.absoluteFilePath();
			recursive_remove_dir(path);
		}
		else
		{
			path = f.fileName();
			d.remove(path);
		}
	}
	
	path = d.dirName();
	d.cdUp();
	d.rmdir(path);
}
