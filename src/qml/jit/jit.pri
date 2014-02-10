include(../../3rdparty/masm/masm-defs.pri)

INCLUDEPATH += $$PWD
INCLUDEPATH += $$OUT_PWD

HEADERS += \
    $$PWD/qv4regalloc_p.h \
    $$PWD/qv4isel_masm_p.h \

SOURCES += \
    $$PWD/qv4regalloc.cpp \
    $$PWD/qv4isel_masm.cpp \

include(../../3rdparty/masm/masm.pri)
