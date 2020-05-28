# Generated from qmldbg_debugger.pro.

#####################################################################
## QQmlDebuggerServiceFactory Plugin:
#####################################################################

qt_internal_add_plugin(QQmlDebuggerServiceFactory
    OUTPUT_NAME qmldbg_debugger
    TYPE qmltooling
    SOURCES
        qqmldebuggerservicefactory.cpp qqmldebuggerservicefactory.h
        qqmlenginedebugservice.cpp qqmlenginedebugservice.h
        qqmlwatcher.cpp qqmlwatcher.h
        qv4datacollector.cpp qv4datacollector.h
        qv4debugger.cpp qv4debugger.h
        qv4debuggeragent.cpp qv4debuggeragent.h
        qv4debugjob.cpp qv4debugjob.h
        qv4debugservice.cpp qv4debugservice.h
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_debugger.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qqmldebuggerservice.json"
