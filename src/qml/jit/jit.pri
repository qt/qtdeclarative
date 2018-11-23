INCLUDEPATH += $$PWD
INCLUDEPATH += $$OUT_PWD

SOURCES += \
    $$PWD/qv4baselinejit.cpp \
    $$PWD/qv4baselineassembler.cpp \
    $$PWD/qv4assemblercommon.cpp

HEADERS += \
    $$PWD/qv4baselinejit_p.h \
    $$PWD/qv4baselineassembler_p.h \
    $$PWD/qv4assemblercommon_p.h

qtConfig(qml-tracing) {
SOURCES += \
    $$PWD/qv4ir.cpp \
    $$PWD/qv4operation.cpp \
    $$PWD/qv4node.cpp \
    $$PWD/qv4graph.cpp \
    $$PWD/qv4graphbuilder.cpp \
    $$PWD/qv4lowering.cpp \
    $$PWD/qv4tracingjit.cpp

HEADERS += \
    $$PWD/qv4ir_p.h \
    $$PWD/qv4operation_p.h \
    $$PWD/qv4runtimesupport_p.h \
    $$PWD/qv4node_p.h \
    $$PWD/qv4graph_p.h \
    $$PWD/qv4graphbuilder_p.h \
    $$PWD/qv4lowering_p.h
}
