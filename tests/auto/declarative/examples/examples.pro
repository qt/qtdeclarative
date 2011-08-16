load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative qtquick1
macx:CONFIG -= app_bundle

SOURCES += tst_examples.cpp 

include(../../../../tools/qmlviewer/qml.pri)

include(../symbianlibs.pri)

symbian: {
    importFiles.files = data
    importFiles.path = .
    DEPLOYMENT += importFiles
} else {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

CONFIG += parallel_test

QT += core-private gui-private v8-private declarative-private qtquick1-private

qpa:CONFIG+=insignificant_test  # QTBUG-20990, aborts
