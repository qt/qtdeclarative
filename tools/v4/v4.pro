TEMPLATE = app
QT = v4-private core-private qmldevtools-private
SOURCES = main.cpp

include(../../src/v4/v4.pri)

llvm-libs {
    DEFINES += QMLJS_WITH_LLVM
}
load(qt_tool)
