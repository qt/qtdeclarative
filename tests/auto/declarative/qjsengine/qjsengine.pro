CONFIG += testcase
TARGET = tst_qjsengine
QT += declarative widgets testlib
macx:CONFIG -= app_bundle
SOURCES += tst_qjsengine.cpp
#temporary
CONFIG += insignificant_test
wince* {
    addFiles.files = script
    addFiles.path = .
    DEPLOYMENT += addFiles
    DEFINES += SRCDIR=\\\"./\\\"
} else {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}
