TARGET = qmldbg_tcp
QT = qml-private core-private network

PLUGIN_TYPE = qmltooling
PLUGIN_CLASS_NAME = QTcpServerConnection
load(qt_plugin)

SOURCES += \
    $$PWD/qtcpserverconnection.cpp \
    $$PWD/../shared/qpacketprotocol.cpp

HEADERS += \
    $$PWD/qtcpserverconnection.h \
    $$PWD/../shared/qpacketprotocol.h

INCLUDEPATH += $$PWD \
    $$PWD/../shared
