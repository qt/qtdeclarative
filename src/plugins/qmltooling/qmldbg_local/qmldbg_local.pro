TARGET = qmldbg_local
QT = qml-private core-private

PLUGIN_TYPE = qmltooling
PLUGIN_CLASS_NAME = QLocalClientConnection
load(qt_plugin)

SOURCES += \
    $$PWD/qlocalclientconnection.cpp \
    $$PWD/../shared/qpacketprotocol.cpp

HEADERS += \
    $$PWD/qlocalclientconnection.h \
    $$PWD/../shared/qqmldebugserver.h \
    $$PWD/../shared/qqmldebugserverconnection.h \
    $$PWD/../shared/qpacketprotocol.h

INCLUDEPATH += $$PWD \
    $$PWD/../shared
