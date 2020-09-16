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

#include <QLocale>
#include <QTranslator>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "qt-helper/qt-helper.h"
#include "mainwin.h"

static void usage(void);

int
main(int argc, char *argv[])
{
	int  ch;
	bool askpass;
	char *user, *msg, *cmd;

	askpass = false;
	msg = user = cmd = NULL;
	while ((ch = getopt(argc, argv, "ac:m:u:h")) != -1)
		switch (ch) {
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
	if (!askpass && argc - optind == 0)
		usage();
	if (askpass && msg != NULL)
		usage();
	if (!askpass)
		cmd = argv[optind];
	QApplication app(argc, argv);
	QTranslator translator;

	/* Set application name and RESOURCE_NAME env to set WM_CLASS */
	QApplication::setApplicationName(PROGRAM);
	(void)qputenv("RESOURCE_NAME", PROGRAM);

	if (translator.load(QLocale(), QLatin1String(PROGRAM),
	    QLatin1String("_"), QLatin1String(LOCALE_PATH)))
		app.installTranslator(&translator);
	if (getuid() == 0 || geteuid() == 0)
		qh_errx(NULL, EXIT_FAILURE, "Refusing to run as root");

	if (askpass) {
		user = dsbsu_get_username();
		MainWin w(cmd, user);
		return (app.exec());
	}
	if (!dsbsu_validate_user(user)) {
		if (dsbsu_error() == DSBSU_ENOUSER)
			qh_errx(NULL, EXIT_FAILURE,
			    QObject::tr("No such user %1").arg(user));
		qh_errx(NULL, EXIT_FAILURE, "%s", dsbsu_strerror());
	}
	if (dsbsu_is_me(user)) {
		switch (system(cmd)) {
		case  -1:
			qh_err(NULL, EXIT_FAILURE, "system(%s)", cmd);
		case 127:
			qh_errx(NULL, EXIT_FAILURE, "Failed to execute shell.");
		}
		return (EXIT_SUCCESS);
	}
	MainWin w(msg, user, cmd);

	if (app.exec() != -1) {
		if (w.proc != NULL && dsbsu_wait(w.proc) != 0) {
			if (dsbsu_error() == DSBSU_EEXECCMD) {
				qh_errx(NULL, EXIT_FAILURE,
				    QObject::tr(
					"Failed to execute command '%1'"
				    ).arg(cmd));
			} else {
				qh_errx(NULL, EXIT_FAILURE, "%s",
				    dsbsu_strerror());
			}
		} else if (w.proc == NULL) {
			switch (dsbsu_error()) {
			case DSBSU_ENOUSER:
				qh_errx(NULL, EXIT_FAILURE,
				   QObject::tr("No such user %1").arg(user));
			case DSBSU_ETIMEOUT:
				qh_errx(NULL, EXIT_FAILURE,
				QObject::tr("su timed out"));
			case DSBSU_EEXECSU:
				qh_errx(NULL, EXIT_FAILURE,
				    QObject::tr("Failed to execute su"));
			default:
				qh_errx(NULL, EXIT_FAILURE, "%s",
				    dsbsu_strerror());
			}
		}
		return (EXIT_SUCCESS);
	}
	return (EXIT_FAILURE);
}

static void
usage()
{
	(void)printf("Usage: %s [-m message][-u user] command\n" \
		     "       %s -a [-c command]\n", PROGRAM, PROGRAM);
	exit(EXIT_FAILURE);
}

