/*-
 * Copyright (c) 2024 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include <QBoxLayout>
#include <QDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

#include "libdsbsu.h"

class MainWin : public QMainWindow {
  Q_OBJECT
 public:
  MainWin(const char *msg, const char *user, const char *cmd,
          QWidget *parent = 0);
  MainWin(const char *msg, const char *user, QWidget *parent = 0);
 private slots:
  void cbCancel();
  void callBackend();
  void resetStatusBar(const QString &);
  void closeEvent(QCloseEvent *);
  void printPwd(void);
  void clearInput(QLineEdit *);

 public:
  dsbsu_proc *proc = 0;

 private:
  QLabel *statusMsg;
  QStatusBar *statusBar;
  QLineEdit *pwdField;
  const char *cmd, *usr;
};
