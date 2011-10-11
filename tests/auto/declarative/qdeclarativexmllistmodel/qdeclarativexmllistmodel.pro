CONFIG += testcase
TARGET = tst_qdeclarativexmllistmodel
contains(QT_CONFIG,xmlpatterns) {
    QT += xmlpatterns
    DEFINES += QTEST_XMLPATTERNS
}
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativexmllistmodel.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private network testlib
