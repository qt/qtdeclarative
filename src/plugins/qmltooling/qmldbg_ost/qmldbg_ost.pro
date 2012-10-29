TARGET = qmldbg_ost
QT       += qml network

PLUGIN_TYPE = qmltooling
load(qt_plugin)

DESTDIR  = $$QT.qml.plugins/qmltooling

SOURCES += \
    qmlostplugin.cpp \
    qostdevice.cpp

HEADERS += \
    qmlostplugin.h \
    qostdevice.h \
    usbostcomm.h

target.path += $$[QT_INSTALL_PLUGINS]/qmltooling
INSTALLS += target
