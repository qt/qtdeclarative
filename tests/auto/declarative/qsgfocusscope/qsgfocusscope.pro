load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative
SOURCES += tst_qsgfocusscope.cpp
macx:CONFIG -= app_bundle

symbian: {
    importFiles.files = data
    importFiles.path = .
    DEPLOYMENT += importFiles
} else {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

QT += core-private gui-private declarative-private

qpa:contains(QT_CONFIG,xcb):CONFIG+=insignificant_test  # QTBUG-21054, unstable
