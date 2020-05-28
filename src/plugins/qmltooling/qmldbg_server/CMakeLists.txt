# Generated from qmldbg_server.pro.

#####################################################################
## QQmlDebugServerFactory Plugin:
#####################################################################

qt_internal_add_plugin(QQmlDebugServerFactory
    OUTPUT_NAME qmldbg_server
    TYPE qmltooling
    SOURCES
        qqmldebugserver.cpp
        qqmldebugserverfactory.h
    PUBLIC_LIBRARIES
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_server.pro:<TRUE>:
# OTHER_FILES = "qqmldebugserver.json"
