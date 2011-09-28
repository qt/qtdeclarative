load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative
QT += network
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeimageprovider.cpp

# QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage
# LIBS += -lgcov

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private declarative-private
