CONFIG += testcase
TARGET = tst_qqmldelegatemodel
macos:CONFIG -= app_bundle

QT += qml testlib core-private qml-private qmlmodels-private

SOURCES += tst_qqmldelegatemodel.cpp

include (../../shared/util.pri)

TESTDATA = data/*

OTHER_FILES += data/*
