QT = core-private qmldevtools-private
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
    qmljs_engine.cpp \
    qmljs_environment.cpp \
    qmljs_runtime.cpp \
    qmljs_value.cpp \
    qv4syntaxchecker.cpp \
    qv4ecmaobjects.cpp \
    qv4isel_masm.cpp \
    llvm_runtime.cpp \
    qv4isel_p.cpp \
    debugging.cpp \
    qv4mm.cpp \
    qv4managed.cpp \
    qv4array.cpp \
    qv4argumentsobject.cpp \
    qv4dateobject.cpp \
    qv4errorobject.cpp \
    qv4functionobject.cpp \
    qv4globalobject.cpp \
    qv4jsonobject.cpp \
    qv4mathobject.cpp \
    qv4object.cpp \
    qv4regexpobject.cpp \
    qv4stringobject.cpp \
    qv4string.cpp \
    qv4objectiterator.cpp \
    qv4regexp.cpp

HEADERS += \
    qv4codegen_p.h \
    qv4ir_p.h \
    qmljs_engine.h \
    qmljs_environment.h \
    qmljs_runtime.h \
    qmljs_math.h \
    qmljs_value.h \
    qv4syntaxchecker_p.h \
    qv4ecmaobjects_p.h \
    qv4isel_masm_p.h \
    qv4isel_p.h \
    qv4isel_util_p.h \
    debugging.h \
    qv4mm.h \
    qv4managed.h \
    qv4array.h \
    qv4argumentsobject.h \
    qv4dateobject.h \
    qv4errorobject.h \
    qv4functionobject.h \
    qv4globalobject.h \
    qv4jsonobject.h \
    qv4mathobject.h \
    qv4object.h \
    qv4regexpobject.h \
    qv4stringobject.h \
    qv4string.h \
    qv4propertydescriptor.h \
    qv4propertytable.h \
    qv4objectiterator.h \
    qv4regexp.h

llvm {

SOURCES += \
    qv4isel_llvm.cpp

HEADERS += \
    qv4isel_llvm_p.h \
    qv4_llvm_p.h

LLVM_RUNTIME_BC = $$PWD/llvm_runtime.bc
DEFINES += LLVM_RUNTIME="\"\\\"$$LLVM_RUNTIME_BC\\\"\""

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
gen_llvm_runtime.commands = clang -O2 -emit-llvm -c $(INCPATH) $$GEN_LLVM_RUNTIME_FLAGS -DQMLJS_LLVM_RUNTIME llvm_runtime.cpp -o $$LLVM_RUNTIME_BC


} else {

DEFINES += QMLJS_NO_LLVM

}

TESTSCRIPT=$$PWD/tests/test262.py
V4CMD = $$OUT_PWD/v4

checktarget.target = check
checktarget.commands = python $$TESTSCRIPT --command=$$V4CMD --parallel --with-test-expectations --update-expectations
checktarget.depends = all
QMAKE_EXTRA_TARGETS += checktarget

checkmothtarget.target = check-interpreter
checkmothtarget.commands = python $$TESTSCRIPT --command=\"$$V4CMD --interpret\" --parallel --with-test-expectations --update-expectations
checkmothtarget.depends = all
QMAKE_EXTRA_TARGETS += checkmothtarget


include(moth/moth.pri)
include(3rdparty/masm/masm.pri)
include(3rdparty/double-conversion/double-conversion.pri)
