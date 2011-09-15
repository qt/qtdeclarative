load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui qtquick1
QT += network
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativetext.cpp

INCLUDEPATH += ../../declarative/shared/
HEADERS += ../../declarative/shared/testhttpserver.h
SOURCES += ../../declarative/shared/testhttpserver.cpp

symbian: {
    importFiles.files = data
    importFiles.path = .
    DEPLOYMENT += importFiles
} else {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

CONFIG += parallel_test

QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private
