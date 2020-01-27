# Generated from qmldbg_preview.pro.

#####################################################################
## QQmlPreviewServiceFactory Plugin:
#####################################################################

qt_add_plugin(QQmlPreviewServiceFactory
    OUTPUT_NAME qmldbg_preview
    TYPE qmltooling
    SOURCES
        qqmlpreviewblacklist.cpp qqmlpreviewblacklist.h
        qqmlpreviewfileengine.cpp qqmlpreviewfileengine.h
        qqmlpreviewfileloader.cpp qqmlpreviewfileloader.h
        qqmlpreviewhandler.cpp qqmlpreviewhandler.h
        qqmlpreviewposition.cpp qqmlpreviewposition.h
        qqmlpreviewservice.cpp qqmlpreviewservice.h
        qqmlpreviewservicefactory.cpp qqmlpreviewservicefactory.h
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Network
        Qt::PacketProtocol
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:qmldbg_preview.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qqmlpreviewservice.json"
