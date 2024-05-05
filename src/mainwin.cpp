/*-
 * Copyright (c) 2024 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "mainwin.h"

#include <QApplication>
#include <QScreen>
#include <QStringConverter>
#include <iostream>

#include "qt-helper/qt-helper.h"

MainWin::MainWin(const char *msg, const char *usr, const char *cmd,
                 QWidget *parent)
    : QMainWindow(parent), cmd{cmd}, usr{usr} {
  QString pstring =
      QString(tr("Please enter the password for "
                 "user %1"));
  QString ustr = QString(usr == 0 ? "root" : usr);
  QString prompt = pstring.arg(ustr);
  QIcon okIcon{qh::loadStockIcon(QStyle::SP_DialogOkButton)};
  QIcon cancelIcon{qh::loadStockIcon(QStyle::SP_DialogCancelButton)};
  QIcon pic{qh::loadIcon(QStringList("dialog-password"))};
  pwdField = new QLineEdit(this);
  statusMsg = new QLabel(this);
  statusBar = new QStatusBar(this);
  QLabel *icon = new QLabel(this);
  QLabel *text;
  QLabel *label = new QLabel(prompt);
  QPushButton *ok = new QPushButton(okIcon, tr("&Ok"));
  QPushButton *cancel = new QPushButton(cancelIcon, tr("&Cancel"));
  QVBoxLayout *vbox = new QVBoxLayout;
  QHBoxLayout *bbox = new QHBoxLayout;
  QHBoxLayout *hbox = new QHBoxLayout;
  QWidget *container = new QWidget(this);

  if (msg == 0 || *msg == '\0') {
    text = new QLabel(
        QString(tr("Execute '%1' as user %2\n").arg(QString(cmd)).arg(ustr)));
  } else
    text = new QLabel(QString(msg).append("\n"));
  icon->setPixmap(pic.pixmap(64));
  text->setWordWrap(true);
  pwdField->setEchoMode(QLineEdit::Password);
  label->setStyleSheet("font-weight: bold;");
  ok->setDefault(true);
  cancel->setDefault(true);

  bbox->addWidget(ok, 1, Qt::AlignRight);
  bbox->addWidget(cancel, 0, Qt::AlignRight);

  hbox->addWidget(icon, 0, Qt::AlignLeft);
  hbox->addWidget(text, 1, Qt::AlignJustify);
  vbox->addLayout(hbox);
  vbox->addWidget(label, 1, Qt::AlignLeft);

  vbox->addWidget(pwdField);
  vbox->addWidget(statusMsg);
  vbox->addLayout(bbox);
  container->setLayout(vbox);
  setCentralWidget(container);

  setMinimumWidth(500);
  setMaximumWidth(500);
  setWindowIcon(pic);
  setWindowTitle("DSBSu");
  show();
  setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(),
                                  qApp->primaryScreen()->geometry()));
  connect(ok, SIGNAL(clicked()), this, SLOT(callBackend()));
  connect(pwdField, SIGNAL(returnPressed()), this, SLOT(callBackend()));
  connect(cancel, SIGNAL(clicked()), this, SLOT(cbCancel()));
  connect(pwdField, SIGNAL(textChanged(const QString &)), this,
          SLOT(resetStatusBar(const QString &)));
}

MainWin::MainWin(const char *cmd, const char *usr, QWidget *parent)
    : QMainWindow(parent), usr{usr} {
  QString pstring =
      QString(tr("Please enter the password for "
                 "user %1"));
  QString ustr = QString(usr == 0 ? "root" : usr);
  QString prompt = pstring.arg(ustr);
  QIcon okIcon{qh::loadStockIcon(QStyle::SP_DialogOkButton)};
  QIcon cancelIcon{qh::loadStockIcon(QStyle::SP_DialogCancelButton)};
  QIcon pic{qh::loadIcon(QStringList("dialog-password"))};
  pwdField = new QLineEdit(this);
  statusMsg = new QLabel(this);
  statusBar = new QStatusBar(this);
  QLabel *icon = new QLabel(this);
  QLabel *text;
  QLabel *label = new QLabel(prompt);
  QPushButton *ok = new QPushButton(okIcon, tr("&Ok"));
  QPushButton *cancel = new QPushButton(cancelIcon, tr("&Cancel"));
  QVBoxLayout *vbox = new QVBoxLayout;
  QHBoxLayout *bbox = new QHBoxLayout;
  QHBoxLayout *hbox = new QHBoxLayout;
  QWidget *container = new QWidget(this);

  text = new QLabel(tr("<qt>The command \"<tt>%1</tt>\" "
                       "requires authentication\n</qt>")
                        .arg(cmd));
  icon->setPixmap(pic.pixmap(64));
  pwdField->setEchoMode(QLineEdit::Password);
  label->setStyleSheet("font-weight: bold;");
  ok->setDefault(true);
  cancel->setDefault(true);

  bbox->addWidget(ok, 1, Qt::AlignRight);
  bbox->addWidget(cancel, 0, Qt::AlignRight);

  hbox->addWidget(icon, 0, Qt::AlignLeft);
  hbox->addWidget(text, 1, Qt::AlignLeft);
  vbox->addLayout(hbox);
  vbox->addWidget(label, 1, Qt::AlignLeft);

  vbox->addWidget(pwdField);
  vbox->addWidget(statusMsg);
  vbox->addLayout(bbox);
  container->setLayout(vbox);
  setCentralWidget(container);

  setMinimumWidth(500);
  setWindowIcon(pic);
  setWindowTitle("DSBSu");
  show();
  setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(),
                                  qApp->primaryScreen()->geometry()));
  connect(ok, SIGNAL(clicked()), this, SLOT(printPwd()));
  connect(pwdField, SIGNAL(returnPressed()), this, SLOT(printPwd()));
  connect(cancel, SIGNAL(clicked()), this, SLOT(cbCancel()));
  connect(pwdField, SIGNAL(textChanged(const QString &)), this,
          SLOT(resetStatusBar(const QString &)));
}

void MainWin::printPwd() {
  auto encoder{QStringEncoder(QStringEncoder::Utf8)};
  QByteArray encstr{encoder(pwdField->text())};

  std::cout << encstr.data() << std::endl;
  clearInput(pwdField);
  encstr.clear();
  QCoreApplication::exit(0);
}

void MainWin::resetStatusBar(const QString & /*unused*/) {
  statusMsg->setText("");
}

void MainWin::callBackend() {
  auto encoder{QStringEncoder(QStringEncoder::Utf8)};
  QByteArray encstr{encoder(pwdField->text())};

  proc = dsbsu_exec_su(cmd, usr, encstr.data());
  clearInput(pwdField);
  encstr.clear();
  if (proc != NULL) QCoreApplication::exit(0);
  if (dsbsu_error() != DSBSU_EAUTH) QCoreApplication::exit(1);
  statusMsg->setText(tr("Wrong password"));
}

void MainWin::cbCancel() {
  clearInput(pwdField);
  QCoreApplication::exit(-1);
}

void MainWin::closeEvent(QCloseEvent * /* unused */) {
  clearInput(pwdField);
  QCoreApplication::exit(-1);
}

void MainWin::clearInput(QLineEdit *input) {
  QString s;
  input->setText(s.fill(' ', 80));
  input->clear();
}