TEMPLATE = app
TARGET   = qmlprofiler
DESTDIR = $$QT.declarative.bins

QT += declarative declarative-private network

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target

macx: CONFIG -= app_bundle

CONFIG   += console declarative_debug

SOURCES += main.cpp \
    qmlprofilerapplication.cpp \
    commandlistener.cpp \
    profileclient.cpp \
    profiledata.cpp

HEADERS += \
    qmlprofilerapplication.h \
    commandlistener.h \
    constants.h \
    profileclient.h \
    profiledata.h
