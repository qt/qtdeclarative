load(qttest_p4)
contains(QT_CONFIG,declarative): QT += declarative
QT += network
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativelanguage.cpp \
           testtypes.cpp
HEADERS += testtypes.h

INCLUDEPATH += ../shared/
HEADERS += ../shared/testhttpserver.h
SOURCES += ../shared/testhttpserver.cpp

importFiles.files = data
importFiles.path = .
DEPLOYMENT += importFiles

CONFIG += parallel_test
QT += core-private gui-private v8-private declarative-private
