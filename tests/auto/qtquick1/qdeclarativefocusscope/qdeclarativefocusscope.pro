CONFIG += testcase
TARGET = tst_qdeclarativefocusscope
SOURCES += tst_qdeclarativefocusscope.cpp
macx:CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private widgets-private declarative-private qtquick1-private testlib
qpa:CONFIG+=insignificant_test  # QTBUG-21013 unstable
