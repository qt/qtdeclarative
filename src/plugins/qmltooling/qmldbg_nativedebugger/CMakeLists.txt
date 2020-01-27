# Generated from qmldbg_nativedebugger.pro.

#####################################################################
## QQmlNativeDebugServiceFactory Plugin:
#####################################################################

qt_add_plugin(QQmlNativeDebugServiceFactory
    OUTPUT_NAME qmldbg_nativedebugger
    TYPE qmltooling
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
