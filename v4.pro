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
    qv4isel.cpp

HEADERS += \
    qv4codegen_p.h \
    qv4ir_p.h \
    qmljs_runtime.h \
    qmljs_objects.h \
    qv4isel_p.h \
    x86-codegen.h \
    amd64-codegen.h



