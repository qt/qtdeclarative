TARGET = qmldbg_server
QT = qml-private core-private

SOURCES += \
    $$PWD/qqmldebugserver.cpp \
    $$PWD/../shared/qpacketprotocol.cpp

HEADERS += \
    $$PWD/qqmldebugserverfactory.h \
    $$PWD/../shared/qqmldebugserver.h \
    $$PWD/../shared/qpacketprotocol.h \
    $$PWD/../shared/qqmldebugserverconnection.h

INCLUDEPATH += $$PWD \
    $$PWD/../shared

OTHER_FILES += \
    qqmldebugserver.json

PLUGIN_TYPE = qmltooling
PLUGIN_CLASS_NAME = QQmlDebugServerFactory
load(qt_plugin)
