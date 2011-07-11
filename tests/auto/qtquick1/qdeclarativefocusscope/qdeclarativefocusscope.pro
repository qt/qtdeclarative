load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative qtquick1
SOURCES += tst_qdeclarativefocusscope.cpp
macx:CONFIG -= app_bundle

symbian: {
    importFiles.files = data
    importFiles.path = .
    DEPLOYMENT += importFiles
} else {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

QT += core-private gui-private declarative-private qtquick1-private
