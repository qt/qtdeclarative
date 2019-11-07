CONFIG += testcase qmltypes
TARGET = tst_qqmltypeloader
QT += qml testlib qml-private quick
macx:CONFIG -= app_bundle

SOURCES += \
    tst_qqmltypeloader.cpp \
    ../../shared/testhttpserver.cpp

HEADERS += \
    ../../shared/testhttpserver.h \
    declarativetesttype.h

QML_IMPORT_VERSION = 3.2
QML_IMPORT_NAME = "declarative.import.for.typeloader.test"

include (../../shared/util.pri)
