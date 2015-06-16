TARGET = qmldbg_tcp
QT = qml-private network

PLUGIN_TYPE = qmltooling
PLUGIN_CLASS_NAME = QTcpServerConnection
load(qt_plugin)

SOURCES += \
    $$PWD/qtcpserverconnection.cpp

HEADERS += \
    $$PWD/qtcpserverconnection.h \
    $$PWD/../shared/qqmldebugserver.h \
    $$PWD/../shared/qqmldebugserverconnection.h

INCLUDEPATH += $$PWD \
    $$PWD/../shared
