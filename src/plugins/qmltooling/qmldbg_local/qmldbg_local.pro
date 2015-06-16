TARGET = qmldbg_local
QT = qml-private

PLUGIN_TYPE = qmltooling
PLUGIN_CLASS_NAME = QLocalClientConnection
load(qt_plugin)

SOURCES += \
    $$PWD/qlocalclientconnection.cpp

HEADERS += \
    $$PWD/qlocalclientconnection.h \
    $$PWD/../shared/qqmldebugserver.h \
    $$PWD/../shared/qqmldebugserverconnection.h

INCLUDEPATH += $$PWD \
    $$PWD/../shared
