# Generated from qmldbg_nativedebugger.pro.

#####################################################################
## qmldbg_nativedebugger Plugin:
#####################################################################

add_qt_plugin(qmldbg_nativedebugger
    TYPE qmltooling
    CLASS_NAME QQmlNativeDebugServiceFactory
    SOURCES
        qqmlnativedebugservice.cpp qqmlnativedebugservice.h
        qqmlnativedebugservicefactory.cpp qqmlnativedebugservicefactory.h
    LIBRARIES
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::PacketProtocol
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:qmldbg_nativedebugger.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qqmlnativedebugservice.json"
# PLUGIN_CLASS_NAME = "QQmlNativeDebugServiceFactory"
# PLUGIN_TYPE = "qmltooling"
# _LOADED = "qt_plugin"
