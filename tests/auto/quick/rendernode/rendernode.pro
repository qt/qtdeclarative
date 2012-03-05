CONFIG += testcase
TARGET = tst_rendernode
SOURCES += tst_rendernode.cpp

macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

include(../../shared/util.pri)

CONFIG += parallel_test
QT += core-private gui-private v8-private qml-private quick-private testlib

OTHER_FILES += \
    data/RenderOrder.qml \
    data/MessUpState.qml \
