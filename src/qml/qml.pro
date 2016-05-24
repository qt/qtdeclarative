TARGET     = QtQml
QT = core-private network

DEFINES   += QT_NO_URL_CAST_FROM_STRING QT_NO_INTEGER_EVENT_COORDINATES

win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x66000000
win32-msvc*:DEFINES *= _CRT_SECURE_NO_WARNINGS
win32:!wince*:!winrt:LIBS += -lshell32
solaris-cc*:QMAKE_CXXFLAGS_RELEASE -= -O2

# Ensure this gcc optimization is switched off for mips platforms to avoid trouble with JIT.
gcc:isEqual(QT_ARCH, "mips"): QMAKE_CXXFLAGS += -fno-reorder-blocks

exists("qqml_enable_gcov") {
    QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage -fno-elide-constructors
    LIBS_PRIVATE += -lgcov
}

greaterThan(QT_GCC_MAJOR_VERSION, 5) {
    # Our code is bad. Temporary workaround.
    QMAKE_CXXFLAGS += -fno-delete-null-pointer-checks
}

QMAKE_DOCS = $$PWD/doc/qtqml.qdocconf

# 2415: variable "xx" of static storage duration was declared but never referenced
intel_icc: WERROR += -ww2415
# unused variable 'xx' [-Werror,-Wunused-const-variable]
greaterThan(QT_CLANG_MAJOR_VERSION, 3)|greaterThan(QT_CLANG_MINOR_VERSION, 3)| \
        greaterThan(QT_APPLE_CLANG_MAJOR_VERSION, 5)| \
        if(equals(QT_APPLE_CLANG_MAJOR_VERSION, 5):greaterThan(QT_APPLE_CLANG_MINOR_VERSION, 0)): \
    WERROR += -Wno-error=unused-const-variable

HEADERS += qtqmlglobal.h \
           qtqmlglobal_p.h

#modules
include(util/util.pri)
include(memory/memory.pri)
include(parser/parser.pri)
include(compiler/compiler.pri)
include(jsapi/jsapi.pri)
include(jit/jit.pri)
include(jsruntime/jsruntime.pri)
include(qml/qml.pri)
include(debugger/debugger.pri)
include(animations/animations.pri)
include(types/types.pri)

MODULE_PLUGIN_TYPES = \
    qmltooling
load(qt_module)
