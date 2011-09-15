load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui qtquick1
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeitem.cpp

symbian: {
    importFiles.files = data
    importFiles.path = .
    DEPLOYMENT += importFiles
} else {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

CONFIG += parallel_test

QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private
qpa:contains(QT_CONFIG,xcb):CONFIG+=insignificant_test  # QTBUG-21012 fails on exit (X11-specific)
