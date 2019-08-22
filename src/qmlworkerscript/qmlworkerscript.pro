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

QMLTYPES_FILENAME = plugins.qmltypes
QMLTYPES_INSTALL_DIR = $$[QT_INSTALL_QML]/QtQml/WorkerScript.2
QML_IMPORT_NAME = QtQml.WorkerScript
IMPORT_VERSION = 2.$$QT_MINOR_VERSION
CONFIG += qmltypes install_qmltypes install_metatypes

load(qt_module)
