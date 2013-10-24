include(../../3rdparty/masm/masm-defs.pri)

INCLUDEPATH += $$PWD
INCLUDEPATH += $$OUT_PWD

HEADERS += \
    $$PWD/qv4compileddata_p.h \
    $$PWD/qv4compiler_p.h \
    $$PWD/qv4codegen_p.h \
    $$PWD/qv4isel_p.h \
    $$PWD/qv4jsir_p.h \
    $$PWD/qv4instr_moth_p.h \
    $$PWD/qv4isel_moth_p.h \
    $$PWD/qv4isel_util_p.h \
    $$PWD/qv4ssa_p.h \
    $$PWD/qv4regalloc_p.h \
    $$PWD/qqmlcodegenerator_p.h \
    $$PWD/qv4isel_masm_p.h

SOURCES += \
    $$PWD/qv4compileddata.cpp \
    $$PWD/qv4compiler.cpp \
    $$PWD/qv4codegen.cpp \
    $$PWD/qv4instr_moth.cpp \
    $$PWD/qv4isel_moth.cpp \
    $$PWD/qv4isel_p.cpp \
    $$PWD/qv4jsir.cpp \
    $$PWD/qv4ssa.cpp \
    $$PWD/qv4regalloc.cpp \
    $$PWD/qqmlcodegenerator.cpp \
    $$PWD/qv4isel_masm.cpp

include(../../3rdparty/masm/masm.pri)
