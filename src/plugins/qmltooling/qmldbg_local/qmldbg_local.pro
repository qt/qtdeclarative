TARGET = qmldbg_local
QT = qml-private core-private

PLUGIN_TYPE = qmltooling
PLUGIN_CLASS_NAME = QLocalClientConnection
load(qt_plugin)

include(qmldbg_local.pri)
