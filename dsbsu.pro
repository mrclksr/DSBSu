PROGRAM = dsbsu

isEmpty(PREFIX) {  
	PREFIX="/usr/local"
}

isEmpty(DATADIR) {  
	DATADIR=$${PREFIX}/share/$${PROGRAM}                                    
}                   

TARGET	     = $${PROGRAM}
HELPER	     = $${PREFIX}/libexec/dsbsu-helper
APPSDIR	     = $${PREFIX}/share/applications
INSTALLS     = target desktopfile locales dsbsudo askpass helper man
CONFIG	    += nostrip
TRANSLATIONS = locale/$${PROGRAM}_de.ts \
               locale/$${PROGRAM}_fr.ts
TEMPLATE     = app
QT	    += widgets
INCLUDEPATH += . lib src
DEFINES     += PROGRAM=\\\"$${PROGRAM}\\\" LOCALE_PATH=\\\"$${DATADIR}\\\"
DEFINES	    += PATH_DSBSU_HELPER=\\\"$${HELPER}\\\"
LIBS	    += -lutil
QMAKE_EXTRA_TARGETS += distclean cleanqm man \
		       dsbsudo askpass cleanscripts cleanman

HEADERS += lib/libdsbsu.h \
	   lib/qt-helper/qt-helper.h \
	   src/mainwin.h

SOURCES += src/main.cpp \
	   src/mainwin.cpp \
	   lib/qt-helper/qt-helper.cpp \
	   lib/libdsbsu.c

qtPrepareTool(LRELEASE, lrelease)
for(a, TRANSLATIONS) {
	cmd = $$LRELEASE $${a}
	system($$cmd)
}

system(sed -E \'s|@INSTALLDIR@|$${PREFIX}/libexec|g\' \
	< dsbsudo.in > dsbsudo; chmod 755 dsbsudo)
system(cp man/$${PROGRAM}.1 man/dsbsudo.1)

target.files      = $${PROGRAM}
target.path       = $${PREFIX}/bin
target.extra	  = strip $${PROGRAM}

desktopfile.path  = $${APPSDIR}
desktopfile.files = $${PROGRAM}.desktop 

man.files      = man/$${PROGRAM}.1 man/dsbsudo.1
man.path       = $${PREFIX}/man/man1

locales.path   = $${DATADIR}
locales.files += locale/*.qm

dsbsudo.path  = $${PREFIX}/bin
dsbsudo.files = dsbsudo

askpass.path  = $${PREFIX}/libexec
askpass.files = dsbsudo-askpass

helper.path   = $${PREFIX}/libexec
helper.files  = dsbsu-helper

cleanqm.commands  = rm -f $${locales.files} 
cleanscripts.commands = rm -f dsbsudo
cleanman.commands = rm -f man/dsbsudo.1
distclean.depends = cleanqm cleanscripts cleanman

