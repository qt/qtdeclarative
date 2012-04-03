load(qt_module)
TARGET     = QtQmlDevTools
QT         = core
TEMPLATE   = lib
DESTDIR    = $$QMAKE_LIBDIR_QT

CONFIG += module
CONFIG += staticlib

DEFINES += QT_BUILD_QMLDEVTOOLS_LIB

load(qt_module_config)

HEADERS += qtqmldevtoolsversion.h

unix|win32-g++*:QMAKE_PKGCONFIG_REQUIRES = QtCore

include(../qml/qml/parser/parser.pri)
