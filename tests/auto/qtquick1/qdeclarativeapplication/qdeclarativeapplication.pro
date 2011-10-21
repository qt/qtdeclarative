CONFIG += testcase
TARGET = tst_qdeclarativeapplication
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeapplication.cpp
QT += core-private gui-private widgets-private declarative-private qtquick1-private testlib
