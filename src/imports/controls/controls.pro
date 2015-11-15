TARGET  = qtlabscontrolsplugin
TARGETPATH = Qt/labs/controls
IMPORT_VERSION = 1.0

QT += qml quick
QT += core-private gui-private qml-private quick-private labstemplates-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

QMAKE_DOCS = $$PWD/doc/qtlabscontrols.qdocconf

OTHER_FILES += \
    qmldir

SOURCES += \
    $$PWD/qtlabscontrolsplugin.cpp

RESOURCES += \
    $$PWD/qtlabscontrolsplugin.qrc

OTHER_FILES += \
    $$PWD/theme.json

include(controls.pri)
include(shared/shared.pri)
include(designer/designer.pri)

CONFIG += no_cxx_module
load(qml_plugin)
