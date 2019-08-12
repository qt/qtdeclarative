# Generated from qmldbg_server.pro.

#####################################################################
## qmldbg_server Plugin:
#####################################################################

add_qt_plugin(qmldbg_server
    TYPE qmltooling
    CLASS_NAME QQmlDebugServerFactory
    SOURCES
        qqmldebugserver.cpp
        qqmldebugserverfactory.h
    LIBRARIES
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::PacketProtocol
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:qmldbg_server.pro:<TRUE>:
# OTHER_FILES = "qqmldebugserver.json"
# PLUGIN_CLASS_NAME = "QQmlDebugServerFactory"
# PLUGIN_TYPE = "qmltooling"
# _LOADED = "qt_plugin"
