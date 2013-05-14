include(llvm_installation.pri)
include(../../../3rdparty/masm/masm-defs.pri)

CONFIG += exceptions

!llvm: DEFINES += QMLJS_NO_LLVM

CONFIG += warn_off

INCLUDEPATH += $$PWD
INCLUDEPATH += $$OUT_PWD

SOURCES += \
    $$PWD/qv4codegen.cpp \
    $$PWD/qv4jsir.cpp \
    $$PWD/qv4engine.cpp \
    $$PWD/qv4context.cpp \
    $$PWD/qv4runtime.cpp \
    $$PWD/qv4value.cpp \
    $$PWD/qv4syntaxchecker.cpp \
    $$PWD/qv4isel_masm.cpp \
    $$PWD/llvm_runtime.cpp \
    $$PWD/qv4isel_p.cpp \
    $$PWD/qv4debugging.cpp \
    $$PWD/qv4lookup.cpp \
    $$PWD/qv4mm.cpp \
    $$PWD/qv4managed.cpp \
    $$PWD/qv4internalclass.cpp \
    $$PWD/qv4sparsearray.cpp \
    $$PWD/qv4arrayobject.cpp \
    $$PWD/qv4argumentsobject.cpp \
    $$PWD/qv4booleanobject.cpp \
    $$PWD/qv4dateobject.cpp \
    $$PWD/qv4errorobject.cpp \
    $$PWD/qv4function.cpp \
    $$PWD/qv4functionobject.cpp \
    $$PWD/qv4globalobject.cpp \
    $$PWD/qv4jsonobject.cpp \
    $$PWD/qv4mathobject.cpp \
    $$PWD/qv4numberobject.cpp \
    $$PWD/qv4object.cpp \
    $$PWD/qv4objectproto.cpp \
    $$PWD/qv4regexpobject.cpp \
    $$PWD/qv4stringobject.cpp \
    $$PWD/qv4variantobject.cpp \
    $$PWD/qv4string.cpp \
    $$PWD/qv4objectiterator.cpp \
    $$PWD/qv4regexp.cpp \
    $$PWD/qv4unwindhelper.cpp \
    $$PWD/qv4v8.cpp \
    $$PWD/qv4executableallocator.cpp

HEADERS += \
    $$PWD/qv4global_p.h \
    $$PWD/qv4codegen_p.h \
    $$PWD/qv4jsir_p.h \
    $$PWD/qv4engine_p.h \
    $$PWD/qv4context_p.h \
    $$PWD/qv4runtime_p.h \
    $$PWD/qv4math_p.h \
    $$PWD/qv4value_p.h \
    $$PWD/qv4syntaxchecker_p.h \
    $$PWD/qv4isel_masm_p.h \
    $$PWD/qv4isel_p.h \
    $$PWD/qv4isel_util_p.h \
    $$PWD/qv4debugging_p.h \
    $$PWD/qv4lookup_p.h \
    $$PWD/qv4identifier_p.h \
    $$PWD/qv4mm_p.h \
    $$PWD/qv4managed_p.h \
    $$PWD/qv4internalclass_p.h \
    $$PWD/qv4sparsearray_p.h \
    $$PWD/qv4arrayobject_p.h \
    $$PWD/qv4argumentsobject_p.h \
    $$PWD/qv4booleanobject_p.h \
    $$PWD/qv4dateobject_p.h \
    $$PWD/qv4errorobject_p.h \
    $$PWD/qv4function_p.h \
    $$PWD/qv4functionobject_p.h \
    $$PWD/qv4globalobject_p.h \
    $$PWD/qv4jsonobject_p.h \
    $$PWD/qv4mathobject_p.h \
    $$PWD/qv4numberobject_p.h \
    $$PWD/qv4object_p.h \
    $$PWD/qv4objectproto_p.h \
    $$PWD/qv4regexpobject_p.h \
    $$PWD/qv4stringobject_p.h \
    $$PWD/qv4variantobject_p.h \
    $$PWD/qv4string_p.h \
    $$PWD/qv4property_p.h \
    $$PWD/qv4objectiterator_p.h \
    $$PWD/qv4regexp_p.h \
    $$PWD/qv4unwindhelper_p.h \
    $$PWD/qv4unwindhelper_p-dw2.h \
    $$PWD/qv4unwindhelper_p-arm.h \
    $$PWD/qv4v8_p.h \
    $$PWD/qcalculatehash_p.h \
    $$PWD/qv4util_p.h \
    $$PWD/qv4executableallocator_p.h

JS_CLASS_SOURCES += $$PWD/qv4dateobject_p.h \
                    $$PWD/qv4stringobject_p.h \
                    $$PWD/qv4booleanobject_p.h \
                    $$PWD/qv4regexpobject_p.h \
                    $$PWD/qv4variantobject_p.h

js_class_bindings.output = ${QMAKE_FILE_BASE}_jsclass.cpp
js_class_bindings.input = JS_CLASS_SOURCES
js_class_bindings.script = $$PWD/v4classgen
js_class_bindings.commands = python $$js_class_bindings.script ${QMAKE_FILE_IN} --output ${QMAKE_FILE_OUT}
js_class_bindings.depends += $$js_class_bindings.script $$PWD/qv4managed_p.h
js_class_bindings.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += js_class_bindings

llvm-libs {

SOURCES += \
    $$PWD/qv4isel_llvm.cpp

HEADERS += \
    $$PWD/qv4isel_llvm_p.h \
    $$PWD/qv4_llvm_p.h

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

ios: DEFINES += ENABLE_ASSEMBLER_WX_EXCLUSIVE=1

include(moth/moth.pri)
include(../../../3rdparty/masm/masm.pri)
include(../../../3rdparty/double-conversion/double-conversion.pri)
