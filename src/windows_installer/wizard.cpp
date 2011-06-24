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
#include "../config.h"

extern bool local_installation;
static QLineEdit * install_location_line = NULL;

static QString get_program_files_location (void)
{
	QString programFilesPath;
	char * var = getenv("PROGRAMFILES");
	programFilesPath = (var == NULL) ? "C:\\Program Files\\" : var;
	
	if (!programFilesPath.endsWith("\\") && !programFilesPath.endsWith("/"))
		((programFilesPath.contains("/")) ? programFilesPath.append("/") : programFilesPath.append("\\"));
	
	return programFilesPath;
}

void QFilePathLine::setPath (const QString & path)
{
	QString fullPath = path;
	
	if (!fullPath.endsWith("\\") && !fullPath.endsWith("/"))
		((fullPath.contains("/")) ? fullPath.append("/") : fullPath.append("\\"));
	
	fullPath.append(PROG_NAME);
	this->setText(fullPath);
}

QFilePathLine::QFilePathLine (const QString & path)
{
	this->setPath(path);
}

void QFilePathLine::mousePressEvent (QMouseEvent * event)
{
	QLineEdit::mousePressEvent(event);
	emit openDialog();
}

WizPageSetupType::WizPageSetupType (void)
{
	this->setTitle("Setup type");
	this->setSubTitle("Choose the installation source.");
	this->radioL = new QRadioButton("&Install version from package");
	this->radioN = new QRadioButton("&Get latest stable version (internet connection required)");
	
	if (local_installation)
		this->radioL->setChecked(true);
	else
	{
		this->radioN->setChecked(true);
		this->radioL->setDisabled(true);
	}
	
	this->vbox = new QVBoxLayout;
	this->vbox->addWidget(this->radioL);
	this->vbox->addWidget(this->radioN);
	this->setLayout(vbox);
}

bool WizPageSetupType::isLocal (void)
{
	return this->radioL->isChecked();
}

bool WizPageSetupType::isRemote (void)
{
	return this->radioN->isChecked();
}

WizPageSetupType::~WizPageSetupType (void)
{
	delete this->radioL;
	delete this->radioN;
	delete this->vbox;
}

WizPageLicense::WizPageLicense (void)
{
	this->setTitle("License");
	this->setSubTitle("Please read the following important information before continuing.");
	
	this->textedit = new QTextEdit;
	this->license = new QTextDocument;
	this->license->setHtml(reinterpret_cast<const char *> (QResource(":license.html").data()));
	this->textedit->setDocument(this->license);
	this->textedit->setReadOnly(true);
	this->vbox = new QVBoxLayout;
	this->vbox->addWidget(textedit);
	this->setLayout(vbox);
}

WizPageLicense::~WizPageLicense (void)
{
	delete this->license;
	delete this->textedit;
	delete this->vbox;
}

WizPageLocation::WizPageLocation (void)
{
	QDialogButtonBox buttonbox;
	QPushButton * open = buttonbox.addButton(QDialogButtonBox::Open);
	
	this->setTitle("Location");
	this->setSubTitle(PROG_NAME " will be installed into the following folder.");
	this->setCommitPage(true);
	
	this->hbox = new QHBoxLayout;
	this->label = new QLabel("Install to:");
	this->line = new QFilePathLine(get_program_files_location());
	this->line->setReadOnly(true);
	this->chooser = new QFileDialog(this->line, Qt::Dialog);
	this->chooser->setDirectory(get_program_files_location());
	this->chooser->setFileMode(QFileDialog::Directory);
	this->chooser->setOption(QFileDialog::ShowDirsOnly, true);
	this->line->connect(this->chooser, SIGNAL(fileSelected(const QString &)), SLOT(setPath(const QString &)));
	this->chooser->connect(this->line, SIGNAL(openDialog()), SLOT(open()));
	this->chooser->connect(open, SIGNAL(clicked()), SLOT(open()));
	this->hbox->addWidget(this->label);
	this->hbox->addWidget(this->line);
	this->hbox->addWidget(open);
	this->setLayout(this->hbox);
	
	install_location_line = this->line;
}

WizPageLocation::~WizPageLocation (void)
{
	delete this->chooser;
	delete this->line;
	delete this->label;
	delete this->hbox;
	
	install_location_line = NULL;
}

WizPageFinal::WizPageFinal (void) : operationsCompleted(false)
{
	this->setTitle("Installation");
	this->setSubTitle("Please wait while " PROG_NAME " is being installed.");
	this->setFinalPage(true);
	
	this->index_downloaded = false;
	this->files_count = 1;
	this->files_downloaded = 0;
	this->vbox = new QVBoxLayout;
	this->label = new QLabel;
	this->pbar = new QProgressBar;
	this->httpbuf = new QBuffer;
	this->vbox->addWidget(this->label);
	this->vbox->addWidget(this->pbar);
	this->setLayout(vbox);
}

WizPageFinal::~WizPageFinal (void)
{
	delete this->label;
	delete this->pbar;
	delete this->vbox;
	delete this->httpbuf;
}

bool WizPageFinal::isComplete (void) const
{
	return this->operationsCompleted;
}

void WizPageFinal::completed (void)
{
	this->operationsCompleted = true;
	emit completeChanged();
}

static int finalPageId = -1;
static QOPSetupWizard * wizardIns;
static WizPageSetupType * wizardPType;

void WizPageFinal::setProgress (int val)
{
	this->setProgress(val, 100);
}

void WizPageFinal::setProgress (int val, int max)
{
	wizardIns->setProgressState(TBPF_NORMAL);
	wizardIns->setProgressValue(val, max);
	this->pbar->setMaximum(max);
	this->pbar->setValue(val);
}

int WizPageFinal::getProgress (void)
{
	return this->getProgress(100);
}

int WizPageFinal::getProgress (int max)
{
	return (this->pbar->value() * max) / this->pbar->maximum();
}

void WizPageFinal::startOperations (int id)
{
	if (id != finalPageId)
		return;
	
	this->pbar->setValue(0);
	this->label->setText("Starting installation...");
	wizardIns->setProgressState(TBPF_INDETERMINATE);
	wizardIns->setCancelable(false);
	if (wizardPType->isRemote())
		get_files_index(this, this->httpbuf);
	else
		this->installFromLocalArchive();
}

void QOPSetupWizard::setCancelable (bool cancelable)
{
	this->cancelable = cancelable;
}

void QOPSetupWizard::reject (void)
{
	if (this->cancelable || QMessageBox::question(this, "Abort Setup", "Setup of the program is not completed. Do you really want to abort the setup?", QMessageBox::Abort | QMessageBox::Cancel) == QMessageBox::Abort)
		QWizard::reject();
}

void WizPageFinal::indexDownloaded (bool error)
{
	if (this->index_downloaded)
		return;
	
	if (error || !parse_files_index(this->httpbuf))
	{
		this->httpbuf->close();
		this->httpbuf->setData("", 0);
		if (get_files_index(this, this->httpbuf))
			return;
		else
		{
			this->httpError();
			
			return;
		}
	}
	
	this->httpbuf->close();
	this->label->setText("Downloading...");
	this->setProgress(0);
	this->index_downloaded = true;
	this->files_count = download_all_files(this);
}

void WizPageFinal::fileDownloaded (int id, bool error)
{
	if (error)
	{
		this->httpError();
		
		return;
	}
	
	this->files_downloaded++;
	this->setProgress(this->files_downloaded * (100 / (this->files_count * 2)));
	
	if (this->files_downloaded == this->files_count)
	{
		finish_downloading();
		this->copyFiles(get_dl_temp_dir());
	}
}

void WizPageFinal::responseHeaderReceived (const QHttpResponseHeader & response)
{
	if (response.statusCode() != 200)
		this->httpError();
}

void WizPageFinal::httpError (void)
{
	if (local_installation)
	{
		if (QMessageBox::question(this, "Unable to download the required files", "Unable to download the required files from remote server. Do you want to continue the installation from a local package?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
		{
			this->installFromLocalArchive();
			return;
		}
	}
	else
		QMessageBox::critical(this, "Unable to download the required files", "Unable to download the required files from remote server.");
	
	QCoreApplication::exit();
}

void WizPageFinal::copyFiles (const QString & dir)
{
	this->label->setText("Copying...");
	recursive_copy_dir(dir, install_location_line->text());
	this->setProgress(100);
	this->label->setText("Installation is complete.");
	this->completed();
}

void WizPageFinal::installFromLocalArchive (void)
{
	extern QString ar_data_root;
	this->setProgress(0);
	this->copyFiles(ar_data_root);
}

void setup_wizard (QOPSetupWizard & wizard)
{
	wizard.setOption(QWizard::NoBackButtonOnLastPage, true);
	wizard.setOption(QWizard::NoBackButtonOnStartPage, true);
	wizard.setWindowIcon(QIcon(":icon.png"));
	wizard.setWizardStyle(QWizard::AeroStyle);
	wizard.setPixmap(QWizard::LogoPixmap, QPixmap(":logo.png"));
	
	QWizardPage * pType = wizardPType = new WizPageSetupType;
	QWizardPage * pLicense = new WizPageLicense;
	QWizardPage * pLocation = new WizPageLocation;
	QWizardPage * pFinal = new WizPageFinal;
	
	pFinal->connect(&wizard, SIGNAL(currentIdChanged(int)), SLOT(startOperations(int)));
	
	wizard.addPage(pType);
	wizard.addPage(pLicense);
	wizard.addPage(pLocation);
	finalPageId = wizard.addPage(pFinal);
	wizardIns = &wizard;
}
