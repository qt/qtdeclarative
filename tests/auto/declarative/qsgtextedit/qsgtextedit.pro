load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui network widgets widgets-private
macx:CONFIG -= app_bundle

SOURCES += tst_qsgtextedit.cpp ../shared/testhttpserver.cpp
HEADERS += ../shared/testhttpserver.h

symbian: {
    importFiles.files = data
    importFiles.path = .
    DEPLOYMENT += importFiles
} else {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}
QT += core-private gui-private v8-private declarative-private
QT += opengl-private

qpa:CONFIG+=insignificant_test  # QTBUG-21010, fails unstably
