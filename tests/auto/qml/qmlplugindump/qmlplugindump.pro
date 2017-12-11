CONFIG += testcase
TARGET = tst_qmlplugindump
QT += testlib gui-private
macx:CONFIG -= app_bundle

DEFINES += QT_QMLTEST_DIR=\\\"$${_PRO_FILE_PWD_}\\\"
SOURCES += tst_qmlplugindump.cpp
