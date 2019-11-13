# Generated from qmlmin.pro.

#####################################################################
## qmlmin Tool:
#####################################################################

add_qt_tool(qmlmin
    TOOLS_TARGET Qml # special case
    SOURCES
        main.cpp
    LIBRARIES
        Qt::QmlDevToolsPrivate
    PUBLIC_LIBRARIES
        Qt::QmlDevTools
)

#### Keys ignored in scope 1:.:.:qmlmin.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML/JS" "Minifier"
# _OPTION = "host_build"
