# Generated from qmldbg_debugger.pro.

#####################################################################
## qmldbg_debugger Plugin:
#####################################################################

add_qt_plugin(qmldbg_debugger
    TYPE qmltooling
    CLASS_NAME QQmlDebuggerServiceFactory
    SOURCES
        qqmldebuggerservicefactory.cpp qqmldebuggerservicefactory.h
        qqmlenginedebugservice.cpp qqmlenginedebugservice.h
        qqmlwatcher.cpp qqmlwatcher.h
        qv4datacollector.cpp qv4datacollector.h
        qv4debugger.cpp qv4debugger.h
        qv4debuggeragent.cpp qv4debuggeragent.h
        qv4debugjob.cpp qv4debugjob.h
        qv4debugservice.cpp qv4debugservice.h
    LIBRARIES
        Qt::CorePrivate
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::PacketProtocol
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:qmldbg_debugger.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qqmldebuggerservice.json"
# _LOADED = "qt_plugin"
