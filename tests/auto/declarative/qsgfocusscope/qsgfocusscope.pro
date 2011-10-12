CONFIG += testcase
TARGET = tst_qsgfocusscope
SOURCES += tst_qsgfocusscope.cpp
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

QT += core-private gui-private declarative-private testlib

qpa:contains(QT_CONFIG,xcb):CONFIG+=insignificant_test  # QTBUG-21054, unstable
