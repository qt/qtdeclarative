TEMPLATE = lib
CONFIG += plugin
QT += qml quick

DESTDIR = Charts
TARGET = chartsplugin

OBJECTS_DIR = tmp
MOC_DIR = tmp

HEADERS += piechart.h \
           pieslice.h \
           chartsplugin.h

SOURCES += piechart.cpp \
           pieslice.cpp \
           chartsplugin.cpp

