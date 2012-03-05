TARGET = qmldbg_ost
QT       += qml network

load(qt_plugin)

QTDIR_build:DESTDIR  = $$QT_BUILD_TREE/plugins/qmltooling
QTDIR_build:REQUIRES += "contains(QT_CONFIG, qml)"

SOURCES += \
    qmlostplugin.cpp \
    qostdevice.cpp

HEADERS += \
    qmlostplugin.h \
    qostdevice.h \
    usbostcomm.h

target.path += $$[QT_INSTALL_PLUGINS]/qmltooling
INSTALLS += target
