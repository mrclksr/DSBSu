PROGRAM = dsbsu

isEmpty(PREFIX) {  
	PREFIX="/usr/local"
}

isEmpty(DATADIR) {  
	DATADIR=$${PREFIX}/share/$${PROGRAM}                                    
}                   

TARGET	     = $${PROGRAM}
APPSDIR	     = $${PREFIX}/share/applications
INSTALLS     = target desktopfile locales scripts
CONFIG	    += nostrip
TRANSLATIONS = locale/$${PROGRAM}_de.ts \
               locale/$${PROGRAM}_fr.ts
TEMPLATE     = app
QT	    += widgets
INCLUDEPATH += . lib src
DEFINES     += PROGRAM=\\\"$${PROGRAM}\\\" LOCALE_PATH=\\\"$${DATADIR}\\\"
LIBS	    += -lutil
QMAKE_EXTRA_TARGETS += distclean cleanqm readme readmemd scripts cleanscripts

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

system(sed -E \'s|@INSTALLDIR@|$${PREFIX}/bin|g\' \
	< dsbsudo.in > dsbsudo; chmod 755 dsbsudo)

target.files      = $${PROGRAM}
target.path       = $${PREFIX}/bin
target.extra	  = strip $${PROGRAM}

desktopfile.path  = $${APPSDIR}
desktopfile.files = $${PROGRAM}.desktop 

readme.target = readme
readme.files = readme.mdoc
readme.commands = mandoc -mdoc readme.mdoc | perl -e \'foreach (<STDIN>) { \
		\$$_ =~ s/(.)\x08\1/\$$1/g; \$$_ =~ s/_\x08(.)/\$$1/g; \
		print \$$_ \
	}\' | sed \'1,1d; \$$,\$$d\' > README

readmemd.target = readmemd
readmemd.files = readme.mdoc
readmemd.commands = mandoc -mdoc -Tmarkdown readme.mdoc | \
			sed -e \'1,1d; \$$,\$$d\' > README.md

locales.path   = $${DATADIR}
locales.files += locale/*.qm

scripts.path  = $${PREFIX}/bin
scripts.files = dsbsudo dsbsu-askpass

cleanqm.commands  = rm -f $${locales.files} 
cleanscripts.commands = rm -f dsbsudo
distclean.depends = cleanqm cleanscripts

