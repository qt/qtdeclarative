# Generated from qmldbg_native.pro.

#####################################################################
## qmldbg_native Plugin:
#####################################################################

add_qt_plugin(qmldbg_native
    TYPE qmltooling
    CLASS_NAME QQmlNativeDebugConnectorFactory
    SOURCES
        qqmlnativedebugconnector.cpp qqmlnativedebugconnector.h
    LIBRARIES
        Qt::CorePrivate
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::PacketProtocol
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:qmldbg_native.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qqmlnativedebugconnector.json"
# _LOADED = "qt_plugin"
