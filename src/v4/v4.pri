include(llvm_installation.pri)
include(../3rdparty/masm/masm-defs.pri)

CONFIG += exceptions

!llvm: DEFINES += QMLJS_NO_LLVM

INCLUDEPATH += $$PWD
