option(host_build)
TARGET     = QtQmlDevTools
QT         = core
CONFIG += static no_module_headers internal_module

MODULE_INCLUDES = \
    \$\$QT_MODULE_INCLUDE_BASE \
    \$\$QT_MODULE_INCLUDE_BASE/QtQml
MODULE_PRIVATE_INCLUDES = \
    \$\$QT_MODULE_INCLUDE_BASE/QtQml/$$QT.qml.VERSION \
    \$\$QT_MODULE_INCLUDE_BASE/QtQml/$$QT.qml.VERSION/QtQml

load(qt_module)

include(../qml/qml/parser/parser.pri)
