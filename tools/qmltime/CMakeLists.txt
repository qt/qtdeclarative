# Generated from qmltime.pro.

#####################################################################
## qmltime Tool:
#####################################################################

qt_get_tool_target_name(target_name qmltime)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Time"
    TOOLS_TARGET Qml # special case
    SOURCES
        qmltime.cpp qmltime.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickPrivate
)

# Don't set properties on a host tool when cross compiling, because it
# is not being built.
if(CMAKE_CROSSCOMPILING AND NOT QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)
    return()
endif()

# Turn the tool into its own self-contained qml module
qt6_add_qml_module(${target_name}
    URI QmlTime
    VERSION 1.0
)
