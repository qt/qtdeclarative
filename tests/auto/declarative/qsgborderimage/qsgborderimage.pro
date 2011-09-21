load(qttest_p4)
macx:CONFIG -= app_bundle

HEADERS += ../shared/testhttpserver.h
SOURCES += tst_qsgborderimage.cpp ../shared/testhttpserver.cpp

symbian: {
    importFiles.files = data
    importFiles.path = .
    DEPLOYMENT += importFiles
} else {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

CONFIG += parallel_test

QT += core-private gui-private declarative-private network widgets

qpa:CONFIG+=insignificant_test  # QTBUG-21004 fails, unstably
