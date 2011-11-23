TARGET = tst_qquickmultipointtoucharea
CONFIG += testcase
macx:CONFIG -= app_bundle

SOURCES += tst_qquickmultipointtoucharea.cpp

importFiles.files = data
importFiles.path = .
DEPLOYMENT += importFiles

QT += core-private gui-private declarative-private quick-private testlib
