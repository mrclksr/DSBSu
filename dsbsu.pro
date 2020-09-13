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
LANGUAGES    = de
TEMPLATE     = app
QT	    += widgets
INCLUDEPATH += . lib src
DEFINES     += PROGRAM=\\\"$${PROGRAM}\\\" LOCALE_PATH=\\\"$${DATADIR}\\\"
LIBS	    += -lutil
QMAKE_EXTRA_TARGETS += distclean cleanqm readme readmemd

HEADERS += lib/libdsbsu.h \
	   lib/qt-helper/qt-helper.h \
	   src/mainwin.h

SOURCES += src/main.cpp \
	   src/mainwin.cpp \
	   lib/qt-helper/qt-helper.cpp \
	   lib/libdsbsu.c

target.files      = $${PROGRAM}
target.path       = $${PREFIX}/bin
target.extra	  = strip $${PROGRAM}

desktopfile.path  = $${APPSDIR}         
desktopfile.files = $${PROGRAM}.desktop 

locales.path = $${DATADIR}

scripts.files	  = dsbsudo dsbsu-askpass
scripts.path	  = $${PREFIX}/bin

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

qtPrepareTool(LRELEASE, lrelease)
for(a, LANGUAGES) {
	in  = locale/$${PROGRAM}_$${a}.ts
	out = locale/$${PROGRAM}_$${a}.qm
	locales.files += $$out
	cmd = $$LRELEASE $$in -qm $$out
	system($$cmd)
}
cleanqm.commands  = rm -f $${locales.files}
distclean.depends = cleanqm

