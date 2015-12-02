TARGET = qtlabstemplatesplugin
TARGETPATH = Qt/labs/templates
IMPORT_VERSION = 1.0

QT += qml quick
QT_PRIVATE += core-private gui-private qml-private quick-private labstemplates-private

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

OTHER_FILES += \
    qmldir

SOURCES += \
    $$PWD/qtlabstemplatesplugin.cpp

CONFIG += no_cxx_module
load(qml_plugin)
