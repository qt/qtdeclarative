TEMPLATE = app
QT = qml-private core-private
SOURCES = main.cpp

include($$PWD/../../src/3rdparty/masm/masm-defs.pri)

CONFIG += exceptions

llvm-libs {
    DEFINES += QMLJS_WITH_LLVM
}
load(qt_tool)
