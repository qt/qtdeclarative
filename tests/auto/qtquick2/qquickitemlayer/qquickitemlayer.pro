CONFIG += testcase
TARGET = tst_qquickitemlayer
SOURCES += tst_qquickitemlayer.cpp

macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

include(../../shared/util.pri)

CONFIG += parallel_test
QT += core-private gui-private v8-private declarative-private quick-private testlib

OTHER_FILES += \
    data/Smooth.qml \
    data/Enabled.qml \
    data/Mipmap.qml \
    data/Effect.qml \
    data/SourceRect.qml \
    data/TextureProvider.qml \
    data/Visible.qml \
    data/ZOrder.qml








