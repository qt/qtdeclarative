TARGET = QtV4
QT_PRIVATE = core-private qmldevtools-private
QT = core

CONFIG += internal_module

include(v4.pri)

load(qt_build_config)
load(qt_module)

CONFIG += warn_off

#win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x66000000 #TODO ??!

!macx-clang*:!win*:LIBS += -rdynamic

SOURCES += \
    qv4codegen.cpp \
    qv4jsir.cpp \
    qv4engine.cpp \
    qv4context.cpp \
    qv4runtime.cpp \
    qv4value.cpp \
    qv4syntaxchecker.cpp \
    qv4isel_masm.cpp \
    llvm_runtime.cpp \
    qv4isel_p.cpp \
    debugging.cpp \
    qv4mm.cpp \
    qv4managed.cpp \
    qv4internalclass.cpp \
    qv4sparsearray.cpp \
    qv4arrayobject.cpp \
    qv4argumentsobject.cpp \
    qv4booleanobject.cpp \
    qv4dateobject.cpp \
    qv4errorobject.cpp \
    qv4functionobject.cpp \
    qv4globalobject.cpp \
    qv4jsonobject.cpp \
    qv4mathobject.cpp \
    qv4numberobject.cpp \
    qv4object.cpp \
    qv4objectproto.cpp \
    qv4regexpobject.cpp \
    qv4stringobject.cpp \
    qv4string.cpp \
    qv4objectiterator.cpp \
    qv4regexp.cpp \
    qv4unwindhelper.cpp \
    qv4v8.cpp \
    qv4executableallocator.cpp

HEADERS += \
    qv4global.h \
    qv4codegen_p.h \
    qv4jsir_p.h \
    qv4engine.h \
    qv4context.h \
    qv4runtime.h \
    qv4math.h \
    qv4value.h \
    qv4syntaxchecker_p.h \
    qv4isel_masm_p.h \
    qv4isel_p.h \
    qv4isel_util_p.h \
    debugging.h \
    qv4identifier.h \
    qv4mm.h \
    qv4managed.h \
    qv4internalclass.h \
    qv4sparsearray.h \
    qv4arrayobject.h \
    qv4argumentsobject.h \
    qv4booleanobject.h \
    qv4dateobject.h \
    qv4errorobject.h \
    qv4functionobject.h \
    qv4globalobject.h \
    qv4jsonobject.h \
    qv4mathobject.h \
    qv4numberobject.h \
    qv4object.h \
    qv4objectproto.h \
    qv4regexpobject.h \
    qv4stringobject.h \
    qv4string.h \
    qv4propertydescriptor.h \
    qv4objectiterator.h \
    qv4regexp.h \
    qv4unwindhelper.h \
    qv4unwindhelper_p-dw2.h \
    qv4unwindhelper_p-arm.h \
    qv4v8.h \
    qcalculatehash_p.h \
    qv4util.h \
    qv4executableallocator.h

llvm-libs {

SOURCES += \
    qv4isel_llvm.cpp

HEADERS += \
    qv4isel_llvm_p.h \
    qv4_llvm_p.h

LLVM_RUNTIME_BC = $$PWD/llvm_runtime.bc
DEFINES += LLVM_RUNTIME="\"\\\"$$LLVM_RUNTIME_BC\\\"\""
DEFINES += QMLJS_WITH_LLVM

INCLUDEPATH += \
    $$system($$LLVM_CONFIG --includedir)

QMAKE_CXXFLAGS += $$system($$LLVM_CONFIG --cppflags) -fvisibility-inlines-hidden
QMAKE_CXXFLAGS -= -pedantic
QMAKE_CXXFLAGS -= -Wcovered-switch-default

LIBS += \
    $$system($$LLVM_CONFIG --ldflags) \
    $$system($$LLVM_CONFIG --libs core jit bitreader linker ipo target x86 arm native)

QMAKE_EXTRA_TARGETS += gen_llvm_runtime

GEN_LLVM_RUNTIME_FLAGS = $$system($$LLVM_CONFIG --cppflags)
GEN_LLVM_RUNTIME_FLAGS -= -pedantic

gen_llvm_runtime.target = llvm_runtime
gen_llvm_runtime.commands = clang -O2 -emit-llvm -c -I$$PWD -I$$PWD/../3rdparty/masm $$join(QT.core.includes, " -I", "-I") $$GEN_LLVM_RUNTIME_FLAGS -DQMLJS_LLVM_RUNTIME llvm_runtime.cpp -o $$LLVM_RUNTIME_BC
}

# Use SSE2 floating point math on 32 bit instead of the default
# 387 to make test results pass on 32 and on 64 bit builds.
linux-g++*:isEqual(QT_ARCH,i386) {
    QMAKE_CFLAGS += -march=pentium4 -msse2 -mfpmath=sse
    QMAKE_CXXFLAGS += -march=pentium4 -msse2 -mfpmath=sse
}

TESTSCRIPT=$$PWD/../../tests/test262.py
V4CMD = v4

checktarget.target = check
checktarget.commands = python $$TESTSCRIPT --command=$$V4CMD --parallel --with-test-expectations --update-expectations
checktarget.depends = all
QMAKE_EXTRA_TARGETS += checktarget

checkmothtarget.target = check-interpreter
checkmothtarget.commands = python $$TESTSCRIPT --command=\"$$V4CMD --interpret\" --parallel --with-test-expectations
checkmothtarget.depends = all
QMAKE_EXTRA_TARGETS += checkmothtarget

linux*|mac {
    LIBS += -ldl
}

debug-with-libunwind {
    UW_INC=$$(LIBUNWIND_INCLUDES)
    isEmpty(UW_INC): error("Please set LIBUNWIND_INCLUDES")
    INCLUDEPATH += $$UW_INC
    UW_LIBS=$$(LIBUNWIND_LIBS)
    isEmpty(UW_LIBS): error("Please set LIBUNWIND_LIBS")
    LIBS += -L$$UW_LIBS
    equals(QT_ARCH, arm): LIBS += -lunwind-arm
    LIBS += -lunwind-dwarf-common -lunwind-dwarf-local -lunwind-elf32 -lunwind
    DEFINES += WTF_USE_LIBUNWIND_DEBUG=1
}

valgrind {
    DEFINES += V4_USE_VALGRIND
}

include(moth/moth.pri)
include(../3rdparty/masm/masm.pri)
include(../3rdparty/double-conversion/double-conversion.pri)
