TEMPLATE = app
QT = v4-private core-private qmldevtools-private
SOURCES = main.cpp

INCLUDEPATH += ../../src/v4
INCLUDEPATH += ../../src/3rdparty/masm
INCLUDEPATH += ../../src/3rdparty/masm/wtf
INCLUDEPATH += ../../src/3rdparty/masm/stubs
INCLUDEPATH += ../../src/3rdparty/masm/stubs/wtf
INCLUDEPATH += ../../src/3rdparty/masm/jit
INCLUDEPATH += ../../src/3rdparty/masm/assembler
INCLUDEPATH += ../../src/3rdparty/masm/disassembler

DEFINES += WTF_EXPORT_PRIVATE="" JS_EXPORT_PRIVATE=""
DEFINES += QMLJS_NO_LLVM
DEFINES += ENABLE_JIT_CONSTANT_BLINDING=0 ENABLE_LLINT=0

load(qt_tool)
