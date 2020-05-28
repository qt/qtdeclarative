# Generated from qmldbg_messages.pro.

#####################################################################
## QDebugMessageServiceFactory Plugin:
#####################################################################

qt_internal_add_plugin(QDebugMessageServiceFactory
    OUTPUT_NAME qmldbg_messages
    TYPE qmltooling
    SOURCES
        qdebugmessageservice.cpp qdebugmessageservice.h
        qdebugmessageservicefactory.cpp qdebugmessageservicefactory.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_messages.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qdebugmessageservice.json"
