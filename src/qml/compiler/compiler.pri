include(../../3rdparty/masm/masm-defs.pri)

INCLUDEPATH += $$PWD
INCLUDEPATH += $$OUT_PWD

HEADERS += \
    $$PWD/qv4codegen_p.h \
    $$PWD/qv4isel_masm_p.h \
    $$PWD/qv4isel_p.h \
    $$PWD/qv4jsir_p.h \
    $$PWD/qv4vme_moth_p.h \
    $$PWD/qv4instr_moth_p.h \
    $$PWD/qv4isel_moth_p.h \
    $$PWD/qv4isel_util_p.h \
    $$PWD/qv4ssa_p.h

SOURCES += \
    $$PWD/qv4codegen.cpp \
    $$PWD/qv4instr_moth.cpp \
    $$PWD/qv4isel_masm.cpp \
    $$PWD/qv4isel_moth.cpp \
    $$PWD/qv4isel_p.cpp \
    $$PWD/qv4jsir.cpp \
    $$PWD/qv4ssa.cpp \
    $$PWD/qv4vme_moth.cpp

include(../../3rdparty/masm/masm.pri)
