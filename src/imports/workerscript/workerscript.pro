CXX_MODULE = qml
TARGET  = workerscriptplugin
TARGETPATH = QtQml/WorkerScript
QML_IMPORT_VERSION = $$QT_VERSION

SOURCES += \
    plugin.cpp

QT = qml-private qmlworkerscript-private

load(qml_plugin)
