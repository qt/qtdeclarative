# Generated from qmllint.pro.

#####################################################################
## qmllint Tool:
#####################################################################

add_qt_tool(qmllint
    TOOLS_TARGET Qml # special case
    SOURCES
        main.cpp
    LIBRARIES
        Qt::QmlDevToolsPrivate
    PUBLIC_LIBRARIES
        Qt::QmlDevTools
)

#### Keys ignored in scope 1:.:.:qmllint.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Syntax" "Verifier"
# _LOADED = "qt_tool"
# _OPTION = "host_build"
