CONFIG += testcase
TARGET = tst_qjsengine
QT += qml widgets testlib
macx:CONFIG -= app_bundle
SOURCES += tst_qjsengine.cpp
wince* {
    addFiles.files = script
    addFiles.path = .
    DEPLOYMENT += addFiles
    DEFINES += SRCDIR=\\\"./\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}
