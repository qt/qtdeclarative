TARGET = qmldbg_server
QT = qml-private packetprotocol-private

PLUGIN_TYPE = qmltooling
PLUGIN_CLASS_NAME = QQmlDebugServerFactory
load(qt_plugin)

SOURCES += \
    $$PWD/qqmldebugserver.cpp \

HEADERS += \
    $$PWD/qqmldebugserverfactory.h \
    $$PWD/../shared/qqmldebugserver.h \
    $$PWD/../shared/qqmldebugserverconnection.h

INCLUDEPATH += $$PWD \
    $$PWD/../shared

OTHER_FILES += \
    qqmldebugserver.json
