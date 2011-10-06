load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative
SOURCES += tst_qsgtargetdirection.cpp
macx:CONFIG -= app_bundle

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += insignificant_test

QT += core-private gui-private v8-private declarative-private opengl-private

