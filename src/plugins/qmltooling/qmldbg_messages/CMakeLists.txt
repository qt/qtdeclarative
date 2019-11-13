# Generated from qmldbg_messages.pro.

#####################################################################
## qmldbg_messages Plugin:
#####################################################################

add_qt_plugin(qmldbg_messages
    TYPE qmltooling
    CLASS_NAME QDebugMessageServiceFactory
    SOURCES
        qdebugmessageservice.cpp qdebugmessageservice.h
        qdebugmessageservicefactory.cpp qdebugmessageservicefactory.h
    LIBRARIES
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::PacketProtocol
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:qmldbg_messages.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qdebugmessageservice.json"
