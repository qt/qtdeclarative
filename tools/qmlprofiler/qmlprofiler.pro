TEMPLATE = app
TARGET   = qmlprofiler
DESTDIR = $$QT.qml.bins

QT += qml qml-private network core-private

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target

macx: CONFIG -= app_bundle

CONFIG   += console declarative_debug

SOURCES += main.cpp \
    qmlprofilerapplication.cpp \
    commandlistener.cpp \
    profileclient.cpp \
    profiledata.cpp \
    qqmldebugclient.cpp

HEADERS += \
    qmlprofilerapplication.h \
    commandlistener.h \
    constants.h \
    profileclient.h \
    profiledata.h \
    qqmldebugclient.h
