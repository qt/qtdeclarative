# Generated from qmldbg_server.pro.

#####################################################################
## QQmlDebugServerFactory Plugin:
#####################################################################

qt_add_plugin(QQmlDebugServerFactory
    OUTPUT_NAME qmldbg_server
    TYPE qmltooling
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
