option(host_build)
TARGET     = QtQmlDevTools
QT         = core-private
CONFIG    += minimal_syncqt internal_module qmldevtools_build

MODULE_INCNAME = QtQml

# 2415: variable "xx" of static storage duration was declared but never referenced
# unused variable 'xx' [-Werror,-Wunused-const-variable]
intel_icc: WERROR += -ww2415
clang:if(greaterThan(QT_CLANG_MAJOR_VERSION, 3)|greaterThan(QT_CLANG_MINOR_VERSION, 3)): \
    WERROR += -Wno-error=unused-const-variable

include(../3rdparty/masm/masm-defs.pri)
include(../qml/parser/parser.pri)
include(../qml/jsruntime/jsruntime.pri)
include(../qml/compiler/compiler.pri)
include(../qml/memory/memory.pri)
include(../qml/jit/jit.pri)

HEADERS += \
    $$PWD/../qml/compiler/qv4isel_p.h \
    $$PWD/../qml/compiler/qv4isel_moth_p.h \
    $$PWD/../qml/compiler/qv4ssa_p.h

SOURCES += \
    $$PWD/../qml/compiler/qv4isel_p.cpp \
    $$PWD/../qml/compiler/qv4isel_moth.cpp \
    $$PWD/../qml/compiler/qv4ssa.cpp

load(qt_module)
