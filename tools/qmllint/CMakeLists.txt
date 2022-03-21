# Generated from qmllint.pro.

#####################################################################
## qmllint Tool:
#####################################################################

qt_get_tool_target_name(target_name qmllint)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Syntax Verifier"
    TOOLS_TARGET Qml # special case
    SOURCES
        main.cpp
        ../shared/qqmltoolingsettings.h
        ../shared/qqmltoolingsettings.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlCompilerPrivate
)
qt_internal_return_unless_building_tools()

add_subdirectory(plugins)

qt_internal_extend_target(${target_name} CONDITION TARGET QmlLintPluginQuick
    DEFINES
        PLUGIN_QUICK_AVAILABLE
    PUBLIC_LIBRARIES
        QmlLintPluginQuick
)

#### Keys ignored in scope 1:.:.:qmllint.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Syntax" "Verifier"
# _OPTION = "host_build"
