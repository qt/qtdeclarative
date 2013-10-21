TEMPLATE = app
QT = qml-private core-private
SOURCES = main.cpp

include($$PWD/../../src/3rdparty/masm/masm-defs.pri)

CONFIG += exceptions

load(qt_tool)
