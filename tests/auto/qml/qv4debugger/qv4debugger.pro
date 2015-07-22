CONFIG += testcase
TARGET = tst_qv4debugger
macx:CONFIG -= app_bundle

SOURCES += \
    $$PWD/tst_qv4debugger.cpp \
    $$PWD/../../../../src/plugins/qmltooling/qmldbg_debugger/qv4datacollector.cpp

HEADERS += \
    $$PWD/../../../../src/plugins/qmltooling/qmldbg_debugger/qv4datacollector.h

INCLUDEPATH += \
    $$PWD/../../../../src/plugins/qmltooling/qmldbg_debugger

QT += core-private gui-private qml-private network testlib
