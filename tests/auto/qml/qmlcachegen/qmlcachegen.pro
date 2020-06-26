CONFIG += testcase qtquickcompiler
TARGET = tst_qmlcachegen
macos:CONFIG -= app_bundle

include (../../shared/util.pri)
TESTDATA = data/*

SOURCES += tst_qmlcachegen.cpp

RESOURCES += \
    data/versionchecks.qml \
    data/jsimport.qml \
    data/script.js \
    data/library.js \
    data/Enums.qml \
    data/componentInItem.qml \
    data/jsmoduleimport.qml \
    data/script.mjs \
    data/module.mjs \
    data/utils.mjs \
    data/parameterAdjustment.qml

workerscripts_test.files = \
    data/worker.js \
    data/worker.qml
workerscripts_test.prefix = /workerscripts

RESOURCES += \
    workerscripts_test \
    trickypaths.qrc \
    data/retain.qrc \
    data/skip.qrc

# QTBUG-46375
!win32: RESOURCES += trickypaths_umlaut.qrc

QTQUICK_COMPILER_SKIPPED_RESOURCES += data/skip.qrc

QT += core-private qml-private testlib
