CONFIG += testcase qtquickcompiler
TARGET = tst_qmlcachegen
macos:CONFIG -= app_bundle

SOURCES += tst_qmlcachegen.cpp

workerscripts_test.files = worker.js worker.qml
workerscripts_test.prefix = /workerscripts
RESOURCES += workerscripts_test

QT += core-private qml-private testlib
