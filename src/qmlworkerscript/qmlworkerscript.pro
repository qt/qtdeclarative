TARGET = QtQmlWorkerScript
QT = core-private qml-private

QMAKE_DOCS = $$PWD/doc/qtqmlworkerscript.qdocconf

DEFINES += QT_NO_URL_CAST_FROM_STRING QT_NO_INTEGER_EVENT_COORDINATES QT_NO_FOREACH

HEADERS += \
    qqmlworkerscriptmodule_p.h \
    qquickworkerscript_p.h \
    qtqmlworkerscriptglobal.h \
    qtqmlworkerscriptglobal_p.h \
    qv4serialize_p.h

SOURCES += \
    qqmlworkerscriptmodule.cpp \
    qquickworkerscript.cpp \
    qv4serialize.cpp

include(../3rdparty/masm/masm-defs.pri)

load(qt_module)
