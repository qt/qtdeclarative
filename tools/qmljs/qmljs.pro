TEMPLATE = app
QT = qml-private core-private
CONFIG += no_import_scan
SOURCES = qmljs.cpp

include($$PWD/../../src/3rdparty/masm/masm-defs.pri)

QMAKE_TARGET_PRODUCT = qmljs
QMAKE_TARGET_DESCRIPTION = QML Javascript tool

load(qt_tool)
