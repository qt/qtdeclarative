TEMPLATE = app
TARGET = qmltestrunner
DESTDIR= $$QT.declarative.bins
CONFIG += warn_on
SOURCES += main.cpp


QT += declarative qmltest

macx: CONFIG -= app_bundle

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target
