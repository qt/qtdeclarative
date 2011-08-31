load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui qtquick1
macx:CONFIG -= app_bundle

include(../../../../tools/qmlviewer/qml.pri)

SOURCES += tst_qdeclarativeviewer.cpp

symbian: {
    include(../symbianlibs.pri)
    importFiles.files = data
    importFiles.path = .
    DEPLOYMENT += importFiles
} else {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

CONFIG += parallel_test
QT += core-private gui-private declarative-private qtquick1-private widgets-private v8-private
