load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative gui
macx:CONFIG -= app_bundle
#temporary
CONFIG += insignificant_test
SOURCES += tst_qdeclarativeapplication.cpp
QT += core-private gui-private declarative-private

