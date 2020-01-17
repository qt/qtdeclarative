CXX_MODULE = qml
TARGET  = qtquick2plugin
TARGETPATH = QtQuick
QML_IMPORT_VERSION = $$QT_VERSION

SOURCES += \
    plugin.cpp

QT += quick-private qml-private qmlmodels-private

qtConfig(qml-worker-script): QT += qmlworkerscript-private

load(qml_plugin)
