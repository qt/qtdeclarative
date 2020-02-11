CONFIG += testcase
TARGET = tst_translation
SOURCES += tst_translation.cpp

macos:CONFIG -= app_bundle

QT += testlib gui-private quicktemplates2-private

include (../shared/util.pri)

TESTDATA = data/*

OTHER_FILES += \
    data/*.qml

RESOURCES += qml_jp.qm qtbase_fr.qm
