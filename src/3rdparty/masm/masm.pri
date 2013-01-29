
HEADERS += $$PWD/assembler/*.h
SOURCES += $$PWD/assembler/ARMAssembler.cpp
SOURCES += $$PWD/assembler/ARMv7Assembler.cpp
SOURCES += $$PWD/assembler/MacroAssemblerARM.cpp
SOURCES += $$PWD/assembler/MacroAssemblerSH4.cpp
SOURCES += $$PWD/assembler/LinkBuffer.cpp

HEADERS += $$PWD/wtf/*.h
SOURCES += $$PWD/wtf/PrintStream.cpp
HEADERS += $$PWD/wtf/PrintStream.h

SOURCES += $$PWD/wtf/FilePrintStream.cpp
HEADERS += $$PWD/wtf/FilePrintStream.h

HEADERS += $$PWD/wtf/RawPointer.h

win32: SOURCES += $$PWD/wtf/OSAllocatorWin.cpp
else: SOURCES += $$PWD/wtf/OSAllocatorPosix.cpp
HEADERS += $$PWD/wtf/OSAllocator.h

SOURCES += $$PWD/wtf/PageAllocationAligned.cpp
HEADERS += $$PWD/wtf/PageAllocationAligned.h
HEADERS += $$PWD/wtf/PageAllocation.h

SOURCES += $$PWD/wtf/PageBlock.cpp
HEADERS += $$PWD/wtf/PageBlock.h

HEADERS += $$PWD/wtf/PageReservation.h

SOURCES += $$PWD/stubs/WTFStubs.cpp
HEADERS += $$PWD/stubs/WTFStubs.h

DEFINES += WTF_EXPORT_PRIVATE="" JS_EXPORT_PRIVATE=""

DEFINES += ENABLE_LLINT=0
DEFINES += ENABLE_DFG_JIT=0
DEFINES += ENABLE_JIT=1
DEFINES += ENABLE_JIT_CONSTANT_BLINDING=0
DEFINES += ENABLE_ASSEMBLER=1

DEFINES += BUILDING_QT__

INCLUDEPATH += $$PWD/jit
INCLUDEPATH += $$PWD/assembler
INCLUDEPATH += $$PWD/runtime
INCLUDEPATH += $$PWD/wtf
INCLUDEPATH += $$PWD/stubs
INCLUDEPATH += $$PWD/stubs/wtf
INCLUDEPATH += $$PWD

DEFINES += WTF_USE_UDIS86=1
INCLUDEPATH += $$PWD/disassembler
INCLUDEPATH += $$PWD/disassembler/udis86
INCLUDEPATH += $$_OUT_PWD
SOURCES += $$PWD/disassembler/Disassembler.cpp
SOURCES += $$PWD/disassembler/UDis86Disassembler.cpp
SOURCES += $$PWD/disassembler/udis86/udis86.c
SOURCES += $$PWD/disassembler/udis86/udis86_decode.c
SOURCES += $$PWD/disassembler/udis86/udis86_input.c
SOURCES += $$PWD/disassembler/udis86/udis86_itab_holder.c
SOURCES += $$PWD/disassembler/udis86/udis86_syn-att.c
SOURCES += $$PWD/disassembler/udis86/udis86_syn.c
SOURCES += $$PWD/disassembler/udis86/udis86_syn-intel.c

DEFINES += ENABLE_YARR_JIT=0
SOURCES += \
    $$PWD/yarr/YarrCanonicalizeUCS2.cpp \
    $$PWD/yarr/YarrInterpreter.cpp \
    $$PWD/yarr/YarrPattern.cpp \
    $$PWD/yarr/YarrSyntaxChecker.cpp

HEADERS += $$PWD/yarr/*.h

retgen.output = RegExpJitTables.h
retgen.script = $$PWD/create_regex_tables
retgen.input = retgen.script
retgen.CONFIG += no_link
retgen.commands = python $$retgen.script > ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += retgen

ITAB = $$PWD/disassembler/udis86/optable.xml
udis86.output = udis86_itab.h
udis86.input = ITAB
udis86.CONFIG += no_link
udis86.commands = python $$PWD/disassembler/udis86/itab.py ${QMAKE_FILE_IN}
QMAKE_EXTRA_COMPILERS += udis86

udis86_tab_cfile.target = $$OUT_PWD/udis86_itab.c
udis86_tab_cfile.depends = udis86_itab.h
QMAKE_EXTRA_TARGETS += udis86_tab_cfile

# Taken from WebKit/Tools/qmake/mkspecs/features/unix/default_post.prf
linux-g++* {
    greaterThan(QT_GCC_MAJOR_VERSION, 3):greaterThan(QT_GCC_MINOR_VERSION, 5) {
        !contains(QMAKE_CXXFLAGS, -std=(c|gnu)\\+\\+(0x|11)) {
            # We need to deactivate those warnings because some names conflicts with upcoming c++0x types (e.g.nullptr).
            QMAKE_CXXFLAGS_WARN_ON += -Wno-c++0x-compat
            QMAKE_CXXFLAGS += -Wno-c++0x-compat
        }
    }
}

# Don't warn about OVERRIDE and FINAL, since they are feature-checked anyways
*clang:!contains(QMAKE_CXXFLAGS, -std=c++11) {
    QMAKE_CXXFLAGS += -Wno-c++11-extensions
    QMAKE_OBJECTIVE_CFLAGS += -Wno-c++11-extensions
}
