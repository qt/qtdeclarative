TARGET = qmldbg_ost
QT       += qml network

PLUGIN_TYPE = qmltooling
PLUGIN_CLASS_NAME = QmlOstPlugin
load(qt_plugin)

SOURCES += \
    qmlostplugin.cpp \
    qostdevice.cpp

HEADERS += \
    qmlostplugin.h \
    qostdevice.h \
    usbostcomm.h
