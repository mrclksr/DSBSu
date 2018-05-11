/*-
 * Copyright (c) 2018 Marcel Kaiser. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <QDesktopWidget>

#include "mainwin.h"
#include "qt-helper/qt-helper.h"

MainWin::MainWin(const char *msg, const char *usr, const char *cmd,
	QWidget *parent) : QMainWindow(parent) {
	this->cmd = cmd;
	this->usr = usr;

	QString prompt	    = QString(tr("Please enter %1's password:")).
				      arg(usr == 0 ? "root" : usr);
	QIcon okIcon	    = qh_loadStockIcon(QStyle::SP_DialogOkButton, 0);
	QIcon cancelIcon    = qh_loadStockIcon(QStyle::SP_DialogCancelButton,
					       NULL);
	QIcon pic	    = qh_loadIcon("dialog-password", NULL);
	pwdField	    = new QLineEdit(this);
	statusMsg	    = new QLabel(this);
	statusBar	    = new QStatusBar(this);
	QLabel	    *icon   = new QLabel(this);	      
	QLabel	    *text   = new QLabel(QString(msg).append("\n"));

	QLabel	    *label  = new QLabel(prompt);
	QPushButton *ok	    = new QPushButton(okIcon, tr("&Ok"));
	QPushButton *cancel = new QPushButton(cancelIcon, tr("&Cancel"));
	QVBoxLayout *vbox   = new QVBoxLayout;
	QHBoxLayout *bbox   = new QHBoxLayout;
	QHBoxLayout *hbox   = new QHBoxLayout;
	QWidget *container  = new QWidget(this);

	icon->setPixmap(pic.pixmap(64));
	text->setWordWrap(true);
	pwdField->setEchoMode(QLineEdit::Password);
	statusBar->addWidget(statusMsg);
	label->setStyleSheet("font-weight: bold;");

	bbox->addWidget(ok,     1, Qt::AlignRight);
        bbox->addWidget(cancel, 0, Qt::AlignRight);

	hbox->addWidget(icon,   0, Qt::AlignLeft);
	hbox->addWidget(text,   1, Qt::AlignJustify);
	vbox->addLayout(hbox);
	vbox->addWidget(label,  1, Qt::AlignLeft);

	vbox->addWidget(pwdField);
	vbox->addLayout(bbox);
	vbox->addWidget(statusBar);
	container->setLayout(vbox);
	setCentralWidget(container);

	setMinimumWidth(500);
	setMaximumWidth(500);
	setWindowIcon(pic);
	setWindowTitle("DSBSu");
	show();
	setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,
	    size(), qApp->desktop()->availableGeometry()));
	connect(ok, SIGNAL(clicked()), this, SLOT(callBackend()));
	connect(pwdField, SIGNAL(returnPressed()), this, SLOT(callBackend()));
	connect(cancel, SIGNAL(clicked()), this, SLOT(cbCancel()));
	connect(pwdField, SIGNAL(textChanged(const QString &)), this,
	    SLOT(resetStatusBar(const QString &)));
}

void
MainWin::resetStatusBar(const QString & /*unused*/)
{
	statusMsg->setText("");
}

void
MainWin::callBackend()
{
	const char *pass;

	pass = pwdField->text().toLatin1();
	if ((proc = dsbsu_exec_su(cmd, usr, pass)) == NULL) {
		if (dsbsu_error() == DSBSU_EAUTH) {
			pwdField->clear();
			statusMsg->setText(tr("Wrong password"));
		} else {
			close();
			QCoreApplication::exit(1);
		}
	} else {
		close();
		QCoreApplication::exit(0);
	}
}

void
MainWin::cbCancel()
{
	QCoreApplication::exit(-1);
}

void MainWin::closeEvent(QCloseEvent */* unused */)
{
	QCoreApplication::exit(-1);
}

