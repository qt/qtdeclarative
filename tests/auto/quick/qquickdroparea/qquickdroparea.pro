TARGET = tst_qquickdroparea
CONFIG += testcase
macx:CONFIG -= app_bundle

SOURCES += tst_qquickdroparea.cpp

OTHER_FILES += $$files(data/*.qml)

include (../../shared/util.pri)
include (../shared/util.pri)

TESTDATA = data/*

QT += core-private gui-private qml-private quick-private network testlib
