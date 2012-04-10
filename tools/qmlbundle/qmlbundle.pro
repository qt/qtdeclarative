TEMPLATE = app
TARGET = qmlbundle
DESTDIR= $$QT.qml.bins

QT       = core qml-private v8-private core-private
CONFIG  += console
CONFIG  -= app_bundle

SOURCES += main.cpp

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target
