# Generated from qmldbg_native.pro.

#####################################################################
## QQmlNativeDebugConnectorFactory Plugin:
#####################################################################

qt_add_plugin(QQmlNativeDebugConnectorFactory
    OUTPUT_NAME qmldbg_native
    TYPE qmltooling
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
