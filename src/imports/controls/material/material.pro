TARGET  = qtquickmaterialstyleplugin
TARGETPATH = Qt/labs/controls/material
IMPORT_VERSION = 1.0

QT += qml quick
QT_PRIVATE += core-private gui-private qml-private quick-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

OTHER_FILES += \
    qmldir

include(material.pri)
include(../shared/shared.pri)

RESOURCES += \
    resources.qrc

CONFIG += no_cxx_module
load(qml_plugin)
