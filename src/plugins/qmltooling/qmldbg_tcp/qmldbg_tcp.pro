load(qt_module)

TARGET = qmldbg_tcp
QT       += qml-private network

load(qt_plugin)

DESTDIR = $$QT.qml.plugins/qmltooling
QTDIR_build:REQUIRES += "contains(QT_CONFIG, qml)"

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
