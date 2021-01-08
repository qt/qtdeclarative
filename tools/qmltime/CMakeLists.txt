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

#### Keys ignored in scope 1:.:.:qmltime.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Time"
# QML_IMPORT_NAME = "QmlTime"
# QML_IMPORT_VERSION = "1.0"

## Scopes:
#####################################################################

# Don't set properties on a host tool when cross compiling, because it
# is not being built.
if(NOT CMAKE_CROSSCOMPILING OR QT_BUILD_TOOLS_WHEN_CROSSCOMPILING)

set_target_properties(${target_name} PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI QmlTime
)
qt6_qml_type_registration(${target_name})
endif()

