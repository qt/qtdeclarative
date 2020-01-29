CXX_MODULE = qml
TARGET  = workerscriptplugin
TARGETPATH = QtQml/WorkerScript
IMPORT_VERSION = 2.15

SOURCES += \
    plugin.cpp

QT = qml-private qmlworkerscript-private

load(qml_plugin)
