TEMPLATE = lib
CONFIG += qt plugin
QT += declarative qtquick1

DESTDIR = lib
OBJECTS_DIR = tmp
MOC_DIR = tmp

HEADERS += piechart.h \
           pieslice.h \
           chartsplugin.h

SOURCES += piechart.cpp \
           pieslice.cpp \
           chartsplugin.cpp
