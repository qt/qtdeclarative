CONFIG += testcase
TARGET = tst_qmlplugindump
QT += testlib gui-private qml
macx:CONFIG -= app_bundle

include(../../shared/util.pri)

SOURCES += tst_qmlplugindump.cpp
