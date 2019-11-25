# Generated from qmlplugindump.pro.

#####################################################################
## qmlplugindump Tool:
#####################################################################

qt_add_tool(qmlplugindump
    TOOLS_TARGET Qml # special case
    SOURCES
        ../shared/qmlstreamwriter.cpp ../shared/qmlstreamwriter.h
        main.cpp
        qmltypereader.cpp qmltypereader.h
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:qmlplugindump.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Plugin" "Metadata" "Dumper"
# QTPLUGIN.platforms = "qminimal"

## Scopes:
#####################################################################

qt_extend_target(qmlplugindump CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)

qt_extend_target(qmlplugindump CONDITION APPLE_OSX
    LINK_OPTIONS
        "-Wl,-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_SOURCE_DIR}/Info.plist" # special case
)
