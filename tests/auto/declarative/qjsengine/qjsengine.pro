load(qttest_p4)
QT += declarative widgets
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
