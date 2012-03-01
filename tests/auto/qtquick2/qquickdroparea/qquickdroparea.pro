TARGET = tst_qquickdroparea
CONFIG += testcase
macx:CONFIG -= app_bundle

SOURCES += tst_qquickdroparea.cpp

CONFIG += parallel_test

QT += core-private gui-private declarative-private quick-private network testlib

mac: CONFIG += insignificant_test # QTBUG-24588
