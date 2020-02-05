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
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
        Qt::PacketProtocolPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:qmldbg_preview.pro:<TRUE>:
# OTHER_FILES = "$$PWD/qqmlpreviewservice.json"
