TARGET = qtquickcontrols2plugin
TARGETPATH = QtQuick/Controls
QML_IMPORT_VERSION = $$QT_VERSION

QT += qml quick
QT_PRIVATE += core-private gui-private qml-private quick-private quicktemplates2-private quickcontrols2-private quickcontrols2impl-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

OTHER_FILES += \
    qmldir

SOURCES += \
    $$PWD/qtquickcontrols2plugin.cpp

qtConfig(quick-designer): include(designer/designer.pri)
include(doc/doc.pri)

CONFIG += no_cxx_module
load(qml_plugin)
