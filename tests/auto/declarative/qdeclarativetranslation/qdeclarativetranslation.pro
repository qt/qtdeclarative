CONFIG += testcase
TARGET = tst_qdeclarativetranslation
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativetranslation.cpp
RESOURCES += data/translation.qrc

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private declarative-private testlib
