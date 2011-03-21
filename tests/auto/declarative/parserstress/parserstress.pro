load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative
macx:CONFIG -= app_bundle

SOURCES += tst_parserstress.cpp

!isEmpty(QT.script.sources) {
    symbian: {
        importFiles.files = $$QT.script.sources\\..\\..\\tests\\auto\\qscriptjstestsuite\\tests
        importFiles.path = .
        DEPLOYMENT += importFiles
        DEFINES += TESTDATADIR=tests
        DEFINES += SRCDIR=.
    } else {
        DEFINES += TESTDATADIR=\\\"$$QT.script.sources/../../tests/auto/qscriptjstestsuite/tests\\\"
        DEFINES += SRCDIR=\\\"$$PWD\\\"
    }
}

CONFIG += parallel_test

