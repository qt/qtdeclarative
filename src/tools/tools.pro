TEMPLATE = app
QT = core v4 v4-private core-private qmldevtools-private
SOURCES = main.cpp

TARGET = v4

INCLUDEPATH += ../v4
INCLUDEPATH += ../3rdparty/masm
INCLUDEPATH += ../3rdparty/masm/wtf
INCLUDEPATH += ../3rdparty/masm/stubs
INCLUDEPATH += ../3rdparty/masm/stubs/wtf
INCLUDEPATH += ../3rdparty/masm/jit
INCLUDEPATH += ../3rdparty/masm/assembler
INCLUDEPATH += ../3rdparty/masm/disassembler

DEFINES += WTF_EXPORT_PRIVATE="" JS_EXPORT_PRIVATE=""
DEFINES += QMLJS_NO_LLVM