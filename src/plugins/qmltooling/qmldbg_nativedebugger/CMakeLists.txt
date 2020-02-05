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
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_nativedebugger.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qqmlnativedebugservice.json"
