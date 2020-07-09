# Generated from qmlpreview.pro.

#####################################################################
## qmlpreview Tool:
#####################################################################

qt_get_tool_target_name(target_name qmlpreview)
qt_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Preview"
    TOOLS_TARGET Qml # special case
    SOURCES
        main.cpp
        qmlpreviewapplication.cpp qmlpreviewapplication.h
        qmlpreviewfilesystemwatcher.cpp qmlpreviewfilesystemwatcher.h
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::QmlDebugPrivate
)

#### Keys ignored in scope 1:.:.:qmlpreview.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Preview"
