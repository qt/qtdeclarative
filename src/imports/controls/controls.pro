TARGET  = qtlabscontrolsplugin
TARGETPATH = Qt/labs/controls
IMPORT_VERSION = 1.0

QT += qml quick
QT += core-private gui-private qml-private quick-private labstemplates-private

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
include(designer/designer.pri)

CONFIG += no_cxx_module
load(qml_plugin)
