CONFIG += testcase

TARGET = tst_qquickaccessible
QT += qml-private network quick-private testlib gui-private
macx:CONFIG -= app_bundle

SOURCES  += tst_qquickaccessible.cpp

include (../../shared/util.pri)

TESTDATA = data/*

OTHER_FILES += data/checkbuttons.qml
OTHER_FILES += data/hittest.qml
OTHER_FILES += data/pushbutton.qml
OTHER_FILES += data/statictext.qml

CONFIG += parallel_test

wince*: {
    accessneeded.files = $$QT.widgets.plugins/accessible/*.dll
    accessneeded.path = accessible
    DEPLOYMENT += accessneeded
}

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
