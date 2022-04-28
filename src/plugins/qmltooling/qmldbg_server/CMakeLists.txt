# Generated from qmldbg_server.pro.

#####################################################################
## QQmlDebugServerFactory Plugin:
#####################################################################

qt_internal_add_plugin(QQmlDebugServerFactoryPlugin
    OUTPUT_NAME qmldbg_server
    CLASS_NAME QQmlDebugServerFactory
    PLUGIN_TYPE qmltooling
    SOURCES
        qqmldebugserverfactory.cpp
        qqmldebugserverfactory.h
    LIBRARIES
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_server.pro:<TRUE>:
# OTHER_FILES = "qqmldebugserver.json"
