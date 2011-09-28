load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative qtquick1
SOURCES += tst_qdeclarativefocusscope.cpp
macx:CONFIG -= app_bundle

DEFINES += SRCDIR=\\\"$$PWD\\\"

QT += core-private gui-private widgets-private declarative-private qtquick1-private
qpa:CONFIG+=insignificant_test  # QTBUG-21013 unstable
