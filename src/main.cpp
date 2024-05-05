/*-
 * Copyright (c) 2024 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <QApplication>
#include <QLocale>
#include <QTranslator>

#include "defs.h"
#include "mainwin.h"
#include "qt-helper/qt-helper.h"

static void usage(void);

int main(int argc, char *argv[]) {
  int ch;
  bool askpass;
  char *user, *msg, *cmd;

  askpass = false;
  msg = user = cmd = NULL;
  while ((ch = getopt(argc, argv, "ac:m:u:h")) != -1) switch (ch) {
      case 'a':
        askpass = true;
        break;
      case 'c':
        cmd = optarg;
        break;
      case 'm':
        msg = optarg;
        break;
      case 'u':
        user = optarg;
        break;
      case '?':
      case 'h':
        usage();
    }
  if (!askpass && argc - optind == 0) usage();
  if (askpass && msg != NULL) usage();
  if (!askpass) cmd = argv[optind];
  QApplication app(argc, argv);
  QTranslator translator;

  /* Set application name and RESOURCE_NAME env to set WM_CLASS */
  QApplication::setApplicationName(PROGRAM);
  (void)qputenv("RESOURCE_NAME", PROGRAM);

  if (translator.load(QLocale(), QLatin1String(PROGRAM), QLatin1String("_"),
                      QLatin1String(LOCALE_PATH)))
    app.installTranslator(&translator);
  if (getuid() == 0 || geteuid() == 0)
    qh::errx(nullptr, EXIT_FAILURE, "Refusing to run as root");

  if (askpass) {
    user = dsbsu_get_username();
    MainWin w(cmd, user);
    return (app.exec());
  }
  if (!dsbsu_validate_user(user)) {
    if (dsbsu_error() == DSBSU_ENOUSER)
      qh::errx(nullptr, EXIT_FAILURE, QObject::tr("No such user %1").arg(user));
    qh::errx(nullptr, EXIT_FAILURE, QString(dsbsu_strerror()));
  }
  if (dsbsu_is_me(user)) {
    switch (system(cmd)) {
      case -1:
        qh::err(nullptr, EXIT_FAILURE, QString("system(%1)").arg(cmd));
      case 127:
        qh::errx(nullptr, EXIT_FAILURE,
                 QObject::tr("Failed to execute shell."));
    }
    return (EXIT_SUCCESS);
  }
  MainWin w(msg, user, cmd);
  int retCode{app.exec()};
  w.hide();

  if (retCode != -1) {
    if (w.proc != NULL && dsbsu_wait(w.proc) != 0) {
      if (dsbsu_error() == DSBSU_EEXECCMD) {
        qh::errx(nullptr, EXIT_FAILURE,
                 QObject::tr("Failed to execute command '%1'").arg(cmd));
      } else {
        qh::errx(nullptr, EXIT_FAILURE, QString(dsbsu_strerror()));
      }
    } else if (w.proc == NULL) {
      switch (dsbsu_error()) {
        case DSBSU_ENOUSER:
          qh::errx(nullptr, EXIT_FAILURE,
                   QObject::tr("No such user %1").arg(user));
        case DSBSU_ETIMEOUT:
          qh::errx(nullptr, EXIT_FAILURE, QObject::tr("su timed out"));
        case DSBSU_EEXECSU:
          qh::errx(nullptr, EXIT_FAILURE, QObject::tr("Failed to execute su"));
        default:
          qh::errx(nullptr, EXIT_FAILURE, QString(dsbsu_strerror()));
      }
    }
    return (EXIT_SUCCESS);
  }
  return (EXIT_FAILURE);
}

static void usage() {
  (void)printf(
      "Usage: %s [-m message][-u user] command\n"
      "       %s -a [-c command]\n",
      PROGRAM, PROGRAM);
  exit(EXIT_FAILURE);
}
