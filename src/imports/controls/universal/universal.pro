TARGET  = qtlabsuniversalstyleplugin
TARGETPATH = Qt/labs/controls/universal
IMPORT_VERSION = 1.0

QT += qml quick
QT += core-private gui-private qml-private quick-private labstemplates-private

OTHER_FILES += \
    qmldir

SOURCES += \
    $$PWD/qtlabsuniversalstyleplugin.cpp

RESOURCES += \
    $$PWD/qtlabsuniversalstyleplugin.qrc

include(universal.pri)
include(../shared/shared.pri)

CONFIG += no_cxx_module
load(qml_plugin)
