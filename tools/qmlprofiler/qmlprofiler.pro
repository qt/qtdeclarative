QT = qml qml-private v8-private network core-private

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

load(qt_tool)
