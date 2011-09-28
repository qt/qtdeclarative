load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative
macx:CONFIG -= app_bundle

SOURCES += tst_parserstress.cpp

!isEmpty(QT.script.sources) {
    DEFINES += TESTDATADIR=\\\"$$QT.script.sources/../../tests/auto/qscriptjstestsuite/tests\\\"
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

CONFIG += parallel_test

QT += core-private gui-private declarative-private
