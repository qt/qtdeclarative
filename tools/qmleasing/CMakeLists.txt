# Generated from qmleasing.pro.

#####################################################################
## qmleasing Binary:
#####################################################################

add_qt_executable(qmleasing
    GUI
    SOURCES
        import.ui
        main.cpp
        mainwindow.cpp mainwindow.h
        pane.ui
        properties.ui
        segmentproperties.cpp segmentproperties.h
        splineeditor.cpp splineeditor.h
    PUBLIC_LIBRARIES
        Qt::Qml
        Qt::Quick
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

# Resources:
add_qt_resource(qmleasing "resources" PREFIX "/" FILES
    Button.qml
    preview.qml)


#### Keys ignored in scope 1:.:.:qmleasing.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Easing" "Curve" "Editor"
# _LOADED = "qt_app"
