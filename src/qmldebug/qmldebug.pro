TARGET    =  QtQmlDebug
QT        =  core-private network packetprotocol-private
CONFIG    += static internal_module

load(qt_module)

SOURCES += \
    qqmldebugclient.cpp \
    qqmldebugconnection.cpp

HEADERS += \
    qqmldebugclient_p.h \
    qqmldebugclient_p_p.h \
    qqmldebugconnection_p.h
