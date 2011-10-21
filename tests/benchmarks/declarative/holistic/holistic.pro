CONFIG += testcase
TEMPLATE = app
TARGET = tst_holistic
QT += declarative network testlib
macx:CONFIG -= app_bundle

CONFIG += release

SOURCES += tst_holistic.cpp \
           testtypes.cpp
HEADERS += testtypes.h

DEFINES += SRCDIR=\\\"$$PWD\\\"
