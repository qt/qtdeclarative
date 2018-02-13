CXX_MODULE = qml
TARGET  = tableviewplugin
TARGETPATH = Qt/labs/tableview
IMPORT_VERSION = 1.0

SOURCES += $$PWD/plugin.cpp

QT += quick-private qml-private

load(qml_plugin)
