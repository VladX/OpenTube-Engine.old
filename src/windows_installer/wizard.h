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

#include <QtGui>
#include <QTranslator>
#include "win7_taskbar.h"

class QFilePathLine : public QLineEdit
{
	Q_OBJECT
	
public:
	QFilePathLine (const QString &);

public slots:
	void setPath (const QString &);

signals:
	void openDialog (void);

protected:
	void mousePressEvent (QMouseEvent * event);
};

class WizPageSetupType : public QWizardPage
{
public:
	WizPageSetupType (void);
	~WizPageSetupType (void);
	bool isLocal (void);
	bool isRemote (void);

private:
	QRadioButton * radioL;
	QRadioButton * radioN;
	QVBoxLayout * vbox;
};

class WizPageLicense : public QWizardPage
{
public:
	WizPageLicense (void);
	~WizPageLicense (void);
	
private:
	QVBoxLayout * vbox;
	QTextEdit * textedit;
	QTextDocument * license;
};

class WizPageLocation : public QWizardPage
{
public:
	WizPageLocation (void);
	~WizPageLocation (void);
	
private:
	QHBoxLayout * hbox;
	QFileDialog * chooser;
	QFilePathLine * line;
	QLabel * label;
};

class WizPageFinal : public QWizardPage
{
	Q_OBJECT
	
public:
	WizPageFinal (void);
	~WizPageFinal (void);
	void setProgress (int);
	void setProgress (int, int);
	bool isComplete (void) const;
	void installFromLocalArchive (void);
	void httpError (void);

public slots:
	void completed (void);
	void startOperations (int);
	void indexDownloaded (bool);
	
private:
	QVBoxLayout * vbox;
	QLabel * label;
	QProgressBar * pbar;
	QBuffer * httpbuf;
	bool operationsCompleted;
};

class QOPSetupWizard : public QWizard
{
public:
	explicit QOPSetupWizard (QWidget * parent = 0);
	void setProgressValue (int value, int max);
	void setProgressState (TBPFLAG state);

#ifdef WIN_TASKBAR_STAFF
protected:
	virtual bool winEvent (MSG *, long *);

private:
	WId mWindowId;
	UINT mTaskbarMessageId;
	ITaskbarList3 * mTaskbar;
#endif
};

void setup_wizard (QOPSetupWizard & wizard);
