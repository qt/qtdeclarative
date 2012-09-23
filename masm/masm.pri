
HEADERS += $$PWD/assembler/*.h
SOURCES += $$PWD/assembler/ARMAssembler.cpp
SOURCES += $$PWD/assembler/ARMv7Assembler.cpp
SOURCES += $$PWD/assembler/MacroAssemblerARM.cpp
SOURCES += $$PWD/assembler/MacroAssemblerSH4.cpp
SOURCES += $$PWD/assembler/LinkBuffer.cpp
SOURCES += $$PWD/stubs/WTFStubs.cpp

DEFINES += NDEBUG
DEFINES += WTF_EXPORT_PRIVATE=""

INCLUDEPATH += $$PWD/jit
INCLUDEPATH += $$PWD/disassembler
INCLUDEPATH += $$PWD/wtf
INCLUDEPATH += $$PWD/stubs
INCLUDEPATH += $$PWD
