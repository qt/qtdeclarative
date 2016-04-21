TARGET = qtquickuniversalstyleplugin
TARGETPATH = QtQuick/Controls.2/Universal
IMPORT_VERSION = 2.0

QT += qml quick
QT_PRIVATE += core-private gui-private qml-private quick-private quicktemplates2-private quickcontrols2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

OTHER_FILES += \
    qmldir

SOURCES += \
    $$PWD/qtquickuniversalstyleplugin.cpp

RESOURCES += \
    $$PWD/qtquickuniversalstyleplugin.qrc

include(universal.pri)

CONFIG += no_cxx_module
load(qml_plugin)
