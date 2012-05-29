load(qt_build_config)

TARGET     = QtQmlDevTools
QT         = core
CONFIG += static

load(qt_module_config)

include(../qml/qml/parser/parser.pri)
