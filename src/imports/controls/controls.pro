TARGET = qtquickcontrolsplugin
TARGETPATH = Qt/labs/controls
IMPORT_VERSION = 1.0

QT += qml quick
QT_PRIVATE += core-private gui-private qml-private quick-private quicktemplates2-private quickcontrols2-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

QMAKE_DOCS = $$PWD/doc/qtquickcontrols2.qdocconf

OTHER_FILES += \
    qmldir

SOURCES += \
    $$PWD/qtquickcontrolsplugin.cpp

RESOURCES += \
    $$PWD/qtquickcontrolsplugin.qrc

include(controls.pri)
!static: include(designer/designer.pri)

qtquickcompiler {
    qmlfiles.prefix = /qt-project.org/imports/Qt/labs/controls
    qmlfiles.files += $$QML_CONTROLS
    RESOURCES += qmlfiles
}

CONFIG += no_cxx_module
load(qml_plugin)
