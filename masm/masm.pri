
HEADERS += $$PWD/assembler/*.h
SOURCES += $$PWD/assembler/ARMAssembler.cpp
SOURCES += $$PWD/assembler/ARMv7Assembler.cpp
SOURCES += $$PWD/assembler/MacroAssemblerARM.cpp
SOURCES += $$PWD/assembler/MacroAssemblerSH4.cpp
SOURCES += $$PWD/assembler/LinkBuffer.cpp

SOURCES += $$PWD/stubs/WTFStubs.cpp
HEADERS += $$PWD/stubs/WTFStubs.h

DEFINES += WTF_EXPORT_PRIVATE=""

DEFINES += ENABLE_LLINT=0
DEFINES += ENABLE_DFG_JIT=0
DEFINES += ENABLE_JIT=0
DEFINES += ENABLE_ASSEMBLER=1

INCLUDEPATH += $$PWD/jit
INCLUDEPATH += $$PWD/assembler
INCLUDEPATH += $$PWD/wtf
INCLUDEPATH += $$PWD/stubs
INCLUDEPATH += $$PWD

DEFINES += WTF_USE_UDIS86=1
INCLUDEPATH += $$PWD/disassembler
INCLUDEPATH += $$PWD/disassembler/udis86
SOURCES += $$PWD/disassembler/UDis86Disassembler.cpp
SOURCES += $$PWD/disassembler/udis86/udis86.c
SOURCES += $$PWD/disassembler/udis86/udis86_decode.c
SOURCES += $$PWD/disassembler/udis86/udis86_input.c
SOURCES += $$PWD/disassembler/udis86/udis86_itab_holder.c
SOURCES += $$PWD/disassembler/udis86/udis86_syn-att.c
SOURCES += $$PWD/disassembler/udis86/udis86_syn.c
SOURCES += $$PWD/disassembler/udis86/udis86_syn-intel.c


ITAB = $$PWD/disassembler/udis86/optable.xml
udis86.output = udis86_itab.h
udis86.input = ITAB
udis86.commands = python $$PWD/disassembler/udis86/itab.py ${QMAKE_FILE_IN}
QMAKE_EXTRA_COMPILERS += udis86
