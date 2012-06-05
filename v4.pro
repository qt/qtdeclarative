QT = core qmldevtools-private
CONFIG -= app_bundle
CONFIG += console

DEFINES += __default_codegen__

udis86:LIBS += -ludis86
else:DEFINES += NO_UDIS86

SOURCES += main.cpp \
    qv4codegen.cpp \
    qv4ir.cpp \
    qmljs_runtime.cpp \
    qmljs_objects.cpp \
    qv4isel.cpp \
    qv4syntaxchecker.cpp \
    qv4ecmaobjects.cpp \
    qv4array.cpp \
    qv4isel_x86_64.cpp

HEADERS += \
    qv4codegen_p.h \
    qv4ir_p.h \
    qmljs_runtime.h \
    qmljs_objects.h \
    qv4isel_p.h \
    x86-codegen.h \
    amd64-codegen.h \
    qv4syntaxchecker_p.h \
    qv4ecmaobjects_p.h \
    qv4array_p.h \
    qv4isel_x86_64_p.h


llvm {

SOURCES += \
    qv4isel_llvm.cpp

HEADERS += \
    qv4isel_llvm_p.h

INCLUDEPATH += \
    $$system(llvm-config --includedir)

DEFINES += \
    __STDC_CONSTANT_MACROS \
    __STDC_FORMAT_MACROS \
    __STDC_LIMIT_MACROS

LIBS += \
    $$system(llvm-config --ldflags) \
    $$system(llvm-config --libs core jit bitreader linker ipo)

QMAKE_EXTRA_TARGETS += gen_llvm_runtime

gen_llvm_runtime.target = llvm_runtime
gen_llvm_runtime.commands = clang -emit-llvm -c -DQMLJS_LLVM_RUNTIME llvm_runtime.cpp  -o llvm_runtime.bc


} else {

DEFINES += QMLJS_NO_LLVM

}
