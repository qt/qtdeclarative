TARGET = qtlabscontrolsplugin
TARGETPATH = Qt/labs/controls
IMPORT_VERSION = 1.0

QT += qml quick
QT_PRIVATE += core-private gui-private qml-private quick-private labstemplates-private labscontrols-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

QMAKE_DOCS = $$PWD/doc/qtlabscontrols.qdocconf

OTHER_FILES += \
    qmldir

HEADERS += \
    $$PWD/qquickbusyindicatorring_p.h \
    $$PWD/qquickprogressstrip_p.h

SOURCES += \
    $$PWD/qtlabscontrolsplugin.cpp \
    $$PWD/qquickbusyindicatorring.cpp \
    $$PWD/qquickprogressstrip.cpp

RESOURCES += \
    $$PWD/qtlabscontrolsplugin.qrc

include(controls.pri)
!static: include(designer/designer.pri)

CONFIG += no_cxx_module
load(qml_plugin)
