CONFIG += testcase
TARGET = tst_qqmlecmascript
macx:CONFIG -= app_bundle

SOURCES += tst_qqmlecmascript.cpp \
           testtypes.cpp \
           ../../shared/testhttpserver.cpp
HEADERS += testtypes.h \
           ../../shared/testhttpserver.h
INCLUDEPATH += ../../shared

RESOURCES += qqmlecmascript.qrc

include (../../shared/util.pri)

greaterThan(QT_GCC_MAJOR_VERSION, 5):!qnx {
    # Our code is bad. Temporary workaround. Fixed in 5.8
    QMAKE_CXXFLAGS += -fno-delete-null-pointer-checks -fno-lifetime-dse
}

# QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage
# LIBS += -lgcov

TESTDATA = data/*

QT += core-private gui-private  qml-private network testlib
qtHaveModule(widgets): QT += widgets

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
