TARGET  = qtquickcontrols2plugin
TARGETPATH = QtQuick/Controls.2
IMPORT_VERSION = 2.0

QT += qml quick
QT += core-private gui-private qml-private quick-private labstemplates-private

QMAKE_DOCS = $$PWD/doc/qtquickcontrols2.qdocconf

OTHER_FILES += \
    qmldir

SOURCES += \
    $$PWD/qtquickcontrols2plugin.cpp

RESOURCES += \
    $$PWD/qtquickcontrols2plugin.qrc

OTHER_FILES += \
    $$PWD/theme.json

include(controls.pri)
include(designer/designer.pri)

CONFIG += no_cxx_module
load(qml_plugin)
