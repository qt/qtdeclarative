option(host_build)

QT = core qmldevtools-private

SOURCES += main.cpp \
    componentversion.cpp \
    findunqualified.cpp \
    qcoloroutput.cpp \
    scopetree.cpp \
    typedescriptionreader.cpp

QMAKE_TARGET_DESCRIPTION = QML Syntax Verifier

load(qt_tool)

HEADERS += \
    componentversion.h \
    findunqualified.h \
    metatypes.h \
    qcoloroutput.h \
    scopetree.h \
    typedescriptionreader.h
