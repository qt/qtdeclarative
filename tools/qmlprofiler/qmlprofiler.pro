QT = qml qml-private network core-private packetprotocol-private
CONFIG += no_import_scan

SOURCES += main.cpp \
    qmlprofilerapplication.cpp \
    commandlistener.cpp \
    qqmldebugclient.cpp \
    qmlprofilerdata.cpp \
    qmlprofilerclient.cpp

HEADERS += \
    qmlprofilerapplication.h \
    commandlistener.h \
    constants.h \
    qmlprofilerdata.h \
    qmlprofilerclient.h \
    qmlprofilereventlocation.h \
    qqmldebugclient.h

load(qt_tool)
