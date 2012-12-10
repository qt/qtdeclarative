TARGET = qmldbg_tcp
QT = qml-private network v8-private core-private

PLUGIN_TYPE = qmltooling
PLUGIN_CLASS_NAME = QTcpServerConnection
load(qt_plugin)

SOURCES += \
    qtcpserverconnection.cpp \
    ../shared/qpacketprotocol.cpp

HEADERS += \
    qtcpserverconnection.h \
    ../shared/qpacketprotocol.h

INCLUDEPATH += ../shared

OTHER_FILES += qtcpserverconnection.json
