INCLUDEPATH += $$PWD
INCLUDEPATH += $$OUT_PWD

HEADERS += \
    $$PWD/qv4bytecodegenerator_p.h \
    $$PWD/qv4compiler_p.h \
    $$PWD/qv4compilercontext_p.h \
    $$PWD/qv4compilercontrolflow_p.h \
    $$PWD/qv4compilerglobal_p.h \
    $$PWD/qv4compilerscanfunctions_p.h \
    $$PWD/qv4codegen_p.h \
    $$PWD/qqmlirbuilder_p.h \
    $$PWD/qv4instr_moth_p.h \
    $$PWD/qv4bytecodehandler_p.h \
    $$PWD/qv4util_p.h

SOURCES += \
    $$PWD/qv4bytecodegenerator.cpp \
    $$PWD/qv4compiler.cpp \
    $$PWD/qv4compilercontext.cpp \
    $$PWD/qv4compilerscanfunctions.cpp \
    $$PWD/qv4codegen.cpp \
    $$PWD/qqmlirbuilder.cpp \
    $$PWD/qv4instr_moth.cpp \
    $$PWD/qv4bytecodehandler.cpp

gcc {
    equals(QT_GCC_MAJOR_VERSION, 5): QMAKE_CXXFLAGS += -fno-strict-aliasing
}
