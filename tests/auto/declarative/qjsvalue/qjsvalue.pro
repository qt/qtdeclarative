CONFIG += testcase
TARGET = tst_qjsvalue
macx:CONFIG -= app_bundle
QT += declarative widgets testlib
SOURCES  += tst_qjsvalue.cpp
HEADERS  += tst_qjsvalue.h
