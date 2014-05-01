option(host_build)
TARGET     = QtQmlDevTools
QT         = core
CONFIG += static no_module_headers internal_module qmldevtools_build

MODULE_INCLUDES = \
    \$\$QT_MODULE_INCLUDE_BASE \
    \$\$QT_MODULE_INCLUDE_BASE/QtQml
MODULE_PRIVATE_INCLUDES = \
    \$\$QT_MODULE_INCLUDE_BASE/QtQml/$$QT.qml.VERSION \
    \$\$QT_MODULE_INCLUDE_BASE/QtQml/$$QT.qml.VERSION/QtQml

load(qt_module)

include(../3rdparty/masm/masm-defs.pri)
include(../qml/parser/parser.pri)
include(../qml/jsruntime/jsruntime.pri)
include(../qml/compiler/compiler.pri)
include(../qml/qml/qml.pri)
