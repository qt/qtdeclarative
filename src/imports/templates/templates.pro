TARGET  = qtlabstemplatesplugin
TARGETPATH = Qt/labs/templates
IMPORT_VERSION = 1.0

QT += qml quick
QT += core-private gui-private qml-private quick-private labstemplates-private

OTHER_FILES += \
    qmldir

SOURCES += \
    $$PWD/qtlabstemplatesplugin.cpp

CONFIG += no_cxx_module
load(qml_plugin)
