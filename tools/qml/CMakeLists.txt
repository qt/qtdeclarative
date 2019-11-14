# Generated from qml.pro.

#####################################################################
## qml Tool:
#####################################################################

qt_add_tool(qml
    TOOLS_TARGET Qml # special case
    SOURCES
        conf.h
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Qml
)

# special case begin
set_source_files_properties(
    conf/default.qml
    conf/resizeToItem.qml
    conf/content/resizeItemToWindow.qml
    conf/content/resizeWindowToItem.qml
    PROPERTIES QT_SKIP_QUICKCOMPILER 1
)
# special case end

# Resources:
set(qml_resource_files
    "conf/content/resizeItemToWindow.qml"
    "conf/content/resizeWindowToItem.qml"
    "conf/default.qml"
    "conf/resizeToItem.qml"
    "resources/qml-64.png"
)

qt_add_resource(qml "qml"
    PREFIX
        "/qt-project.org/QmlRuntime"
    FILES
        ${qml_resource_files}
)


#### Keys ignored in scope 1:.:.:qml.pro:<TRUE>:
# ICON = "resources/qml64.png"
# QMAKE_TARGET_DESCRIPTION = "QML" "Runtime"

## Scopes:
#####################################################################

qt_extend_target(qml CONDITION TARGET Qt::Gui
    PUBLIC_LIBRARIES
        Qt::Gui
)

qt_extend_target(qml CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 4:.:.:qml.pro:WIN32:
# RC_ICONS = "resources/qml.ico"

#### Keys ignored in scope 5:.:.:qml.pro:APPLE:
# ICON = "resources/qml.icns"
# OTHER_FILES = "resources/Info.plist"
# QMAKE_INFO_PLIST = "resources/Info.plist"

qt_extend_target(qml CONDITION QT_FEATURE_qml_debug
    DEFINES
        QT_QML_DEBUG_NO_WARNING
)
