CONFIG += testcase
TARGET = tst_qdeclarativelistcompositor
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativelistcompositor.cpp

symbian: {
    importFiles.files = data
    importFiles.path = .
    DEPLOYMENT += importFiles
} else {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

CONFIG += parallel_test

QT += core-private gui-private declarative-private testlib
