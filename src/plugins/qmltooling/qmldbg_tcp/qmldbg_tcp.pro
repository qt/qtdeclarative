load(qt_module)

TARGET = qmldbg_tcp
QT       += qml-private network

load(qt_plugin)

DESTDIR = $$QT.qml.plugins/qmltooling
QTDIR_build:REQUIRES += "contains(QT_CONFIG, qml)"

SOURCES += \
    qtcpserverconnection.cpp

HEADERS += \
    qtcpserverconnection.h

OTHER_FILES += qtcpserverconnection.json

target.path += $$[QT_INSTALL_PLUGINS]/qmltooling
INSTALLS += target
