CXX_MODULE = qml
TARGET  = qtquick2plugin
TARGETPATH = QtQuick.2
IMPORT_VERSION = 2.$$QT_MINOR_VERSION

SOURCES += \
    plugin.cpp

QT += quick-private qml-private qmlmodels-private

qtConfig(qml-worker-script): QT += qmlworkerscript-private

load(qml_plugin)
