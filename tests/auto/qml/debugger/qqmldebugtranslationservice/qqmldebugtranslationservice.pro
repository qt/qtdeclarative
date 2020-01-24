CONFIG += testcase
TARGET = tst_qdebugtranslationservice
QT += network testlib gui-private core-private qmldebug-private
macos:CONFIG -= app_bundle

SOURCES += tst_qqmldebugtranslationservice.cpp

include(../shared/debugutil.pri)

TESTDATA = data/*

OTHER_FILES += data/test.qml
