CXX_MODULE = qml
TARGET  = qmllocalstorageplugin
TARGETPATH = QtQuick/LocalStorage

QT += sql qml qml-private v8-private core-private

SOURCES += plugin.cpp

load(qml_plugin)

OTHER_FILES += localstorage.json
