load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative qtquick1
SOURCES += tst_qdeclarativeanimations.cpp
macx:CONFIG -= app_bundle

symbian: {
    importFiles.files = data
    importFiles.path = .
    DEPLOYMENT += importFiles
} else {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

CONFIG += parallel_test

QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private
