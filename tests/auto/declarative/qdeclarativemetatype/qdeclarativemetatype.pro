load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative widgets
SOURCES += tst_qdeclarativemetatype.cpp
macx:CONFIG -= app_bundle

!symbian: {
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

CONFIG += parallel_test
#temporary
CONFIG += insignificant_test
QT += core-private gui-private declarative-private
