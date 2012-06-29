load(qt_build_config)

TARGET = qmldbg_tcp
QT       += qml-private network v8-private core-private

load(qt_plugin)

DESTDIR = $$QT.qml.plugins/qmltooling

SOURCES += \
    qtcpserverconnection.cpp \
    ../shared/qpacketprotocol.cpp

HEADERS += \
    qtcpserverconnection.h \
    ../shared/qpacketprotocol.h

INCLUDEPATH += ../shared

OTHER_FILES += qtcpserverconnection.json

target.path += $$[QT_INSTALL_PLUGINS]/qmltooling
INSTALLS += target
