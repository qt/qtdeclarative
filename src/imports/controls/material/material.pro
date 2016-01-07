TARGET = qtlabsmaterialstyleplugin
TARGETPATH = Qt/labs/controls/material
IMPORT_VERSION = 1.0

QT += qml quick
QT_PRIVATE += core-private gui-private qml-private quick-private labstemplates-private labscontrols-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

OTHER_FILES += \
    qmldir

SOURCES += \
    $$PWD/qtlabsmaterialstyleplugin.cpp

RESOURCES += \
    $$PWD/qtlabsmaterialstyleplugin.qrc

include(material.pri)

CONFIG += no_cxx_module
load(qml_plugin)
