CONFIG += testcase
TARGET = tst_headersclean
SOURCES  += tst_headersclean.cpp
QT = core testlib

contains(QT_CONFIG,qml): QT += qml qml-private
