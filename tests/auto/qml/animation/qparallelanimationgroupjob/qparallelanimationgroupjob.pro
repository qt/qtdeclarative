CONFIG += testcase
CONFIG += parallel_test
macx:CONFIG -= app_bundle
TARGET = tst_qparallelanimationgroupjob
QT = core-private gui qml-private testlib
SOURCES = tst_qparallelanimationgroupjob.cpp
