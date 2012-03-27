TEMPLATE = app
TARGET   = qmlprofiler
DESTDIR = $$QT.qml.bins

QT += qml qml-private v8-private network core-private

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target

macx: CONFIG -= app_bundle

CONFIG   += console

SOURCES += main.cpp \
    qmlprofilerapplication.cpp \
    commandlistener.cpp \
    qqmldebugclient.cpp \
    qmlprofilerdata.cpp \
    qmlprofilerclient.cpp \
    qpacketprotocol.cpp

HEADERS += \
    qmlprofilerapplication.h \
    commandlistener.h \
    constants.h \
    qmlprofilerdata.h \
    qmlprofilerclient.h \
    qmlprofilereventlocation.h \
    qqmldebugclient.h \
    qpacketprotocol.h
