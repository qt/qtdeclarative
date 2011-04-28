load(qt_module)

TARGET = qmldbg_tcp
QT       += declarative network

include($$QT_SOURCE_TREE/src/plugins/qpluginbase.pri)

DESTDIR = $$QT.declarative.plugins/qmltooling
QTDIR_build:REQUIRES += "contains(QT_CONFIG, declarative)"

SOURCES += \
    qtcpserverconnection.cpp

HEADERS += \
    qtcpserverconnection.h

target.path += $$[QT_INSTALL_PLUGINS]/qmltooling
INSTALLS += target

symbian:TARGET.UID3=0x20031E90
