load(qt_build_config)

TARGET     = QtQml
QT = core-private network

DEFINES   += QT_NO_URL_CAST_FROM_STRING QT_NO_INTEGER_EVENT_COORDINATES

win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x66000000
win32-msvc*:DEFINES *= _CRT_SECURE_NO_WARNINGS
solaris-cc*:QMAKE_CXXFLAGS_RELEASE -= -O2

exists("qqml_enable_gcov") {
    QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage -fno-elide-constructors
    LIBS += -lgcov
}

load(qt_module_config)

QMAKE_DOCS = $$PWD/doc/qtqml.qdocconf

# private dependencies
QT += v8-private

HEADERS += qtqmlglobal.h \
           qtqmlglobal_p.h

#modules
include(util/util.pri)
include(qml/qml.pri)
include(debugger/debugger.pri)
include(animations/animations.pri)
