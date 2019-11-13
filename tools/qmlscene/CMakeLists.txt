# Generated from qmlscene.pro.

#####################################################################
## qmlscene Tool:
#####################################################################

add_qt_tool(qmlscene
    TOOLS_TARGET Qml # special case
    SOURCES
        main.cpp
    DEFINES
        QML_RUNTIME_TESTING
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:qmlscene.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Scene" "Viewer"

## Scopes:
#####################################################################

extend_target(qmlscene CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)

extend_target(qmlscene CONDITION QT_FEATURE_qml_debug
    DEFINES
        QT_QML_DEBUG_NO_WARNING
)
