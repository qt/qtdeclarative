# Generated from qml.pro.

#####################################################################
## qml Tool:
#####################################################################

qt_add_tool(qml
    TARGET_DESCRIPTION "QML Runtime"
    TOOLS_TARGET Qml # special case
    SOURCES
        conf.h
        main.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
)

# special case begin
# Don't set properties on a host tool when cross compiling, because it
# is not being built.
if(CMAKE_CROSSCOMPILING)
    return()
endif()

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
# ICON = "resources/qml-64.png"
# QMAKE_TARGET_DESCRIPTION = "QML" "Runtime"
# QML_IMPORT_NAME = "QmlRuntime.Config"
# QML_IMPORT_VERSION = "1.0"

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

set_target_properties(qml PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI QmlRuntime.Config
)
qt6_qml_type_registration(qml)

