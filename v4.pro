QT = core qmldevtools-private
CONFIG -= app_bundle
CONFIG += console

LLVM_CONFIG=llvm-config

# Pick up the qmake variable or environment variable for LLVM_INSTALL_DIR. If either was set, change the LLVM_CONFIG to use that.
isEmpty(LLVM_INSTALL_DIR):LLVM_INSTALL_DIR=$$(LLVM_INSTALL_DIR)
!isEmpty(LLVM_INSTALL_DIR):LLVM_CONFIG=$$LLVM_INSTALL_DIR/bin/llvm-config

LIBS += -rdynamic

SOURCES += main.cpp \
    qv4codegen.cpp \
    qv4ir.cpp \
    qmljs_runtime.cpp \
    qmljs_objects.cpp \
    qv4syntaxchecker.cpp \
    qv4ecmaobjects.cpp \
    qv4array.cpp \
    qv4isel_masm.cpp \
    llvm_runtime.cpp

HEADERS += \
    qv4codegen_p.h \
    qv4ir_p.h \
    qmljs_runtime.h \
    qmljs_objects.h \
    qmljs_math.h \
    qv4syntaxchecker_p.h \
    qv4ecmaobjects_p.h \
    qv4array_p.h \
    qv4isel_masm_p.h

llvm {

SOURCES += \
    qv4isel_llvm.cpp

HEADERS += \
    qv4isel_llvm_p.h

INCLUDEPATH += \
    $$system($$LLVM_CONFIG --includedir)

DEFINES += \
    __STDC_CONSTANT_MACROS \
    __STDC_FORMAT_MACROS \
    __STDC_LIMIT_MACROS

LIBS += \
    $$system($$LLVM_CONFIG --ldflags) \
    $$system($$LLVM_CONFIG --libs core jit bitreader linker ipo target x86 arm)

QMAKE_EXTRA_TARGETS += gen_llvm_runtime

gen_llvm_runtime.target = llvm_runtime
gen_llvm_runtime.commands = clang -O2 -emit-llvm -c -DQMLJS_LLVM_RUNTIME llvm_runtime.cpp  -o llvm_runtime.bc


} else {

DEFINES += QMLJS_NO_LLVM

}

include(moth/moth.pri)
include(masm/masm.pri)
