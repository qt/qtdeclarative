load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative
macx:CONFIG -= app_bundle

SOURCES += tst_qsgrepeater.cpp

testFiles.files = data
testFiles.path = .
DEPLOYMENT += testFiles

CONFIG += parallel_test
QT += core-private gui-private declarative-private
