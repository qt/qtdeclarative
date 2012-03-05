TEMPLATE = app
TARGET = qmltestrunner
DESTDIR= $$QT.qml.bins
CONFIG += warn_on
SOURCES += main.cpp


QT += qml qmltest

macx: CONFIG -= app_bundle

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target
