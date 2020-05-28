# Generated from qmlscene.pro.

#####################################################################
## qmlscene Tool:
#####################################################################

qt_add_tool(qmlscene
    TARGET_DESCRIPTION "QML Scene Viewer"
    TOOLS_TARGET Qml # special case
    SOURCES
        main.cpp
    DEFINES
        QML_RUNTIME_TESTING
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::Quick
        Qt::QuickPrivate
)

#### Keys ignored in scope 1:.:.:qmlscene.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Scene" "Viewer"

## Scopes:
#####################################################################

qt_extend_target(qmlscene CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)

qt_extend_target(qmlscene CONDITION QT_FEATURE_qml_debug
    DEFINES
        QT_QML_DEBUG_NO_WARNING
)
