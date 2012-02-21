load(qt_module)

TARGET = qmldbg_tcp
QT       += declarative-private network

load(qt_plugin)

DESTDIR = $$QT.declarative.plugins/qmltooling
QTDIR_build:REQUIRES += "contains(QT_CONFIG, declarative)"

SOURCES += \
    qtcpserverconnection.cpp

HEADERS += \
    qtcpserverconnection.h

OTHER_FILES += qtcpserverconnection.json

target.path += $$[QT_INSTALL_PLUGINS]/qmltooling
INSTALLS += target
