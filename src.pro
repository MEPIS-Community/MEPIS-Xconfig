QTDIR = /usr/local/qt4
TEMPLATE = app
TARGET = mxconfig
TRANSLATIONS += mxconfig_ca.ts mxconfig_de.ts mxconfig_es.ts mxconfig_fr.ts \
           mxconfig_pt_BR.ts mxconfig_nl.ts
FORMS += meconfig.ui
HEADERS += mconfig.h 
SOURCES += main.cpp mconfig.cpp
CONFIG += release warn_on thread qt
