CONFIG += testcase
TARGET = tst_parserstress
macx:CONFIG -= app_bundle

SOURCES += tst_parserstress.cpp

!isEmpty(QT.script.sources) {
    DEFINES += TESTDATADIR=\\\"$$QT.script.sources/../../tests/auto/qscriptjstestsuite/tests\\\"
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

CONFIG += parallel_test

QT += core-private gui-private declarative-private testlib
