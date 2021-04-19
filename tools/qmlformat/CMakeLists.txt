# Generated from qmlformat.pro.

#####################################################################
## qmlformat Tool:
#####################################################################

qt_get_tool_target_name(target_name qmlformat)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Formatter"
    TOOLS_TARGET Qml # special case
    SOURCES
        qmlformat.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::QmlDevToolsPrivate
        Qt::QmlDomPrivate
)

#### Keys ignored in scope 1:.:.:qmlformat.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Formatter"
# _OPTION = "host_build"
