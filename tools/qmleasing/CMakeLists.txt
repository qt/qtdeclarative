# Generated from qmleasing.pro.

#####################################################################
## qmleasing Binary:
#####################################################################

qt_add_executable(qmleasing
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
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

# Resources:
set(resources_resource_files
    "Button.qml"
    "preview.qml"
)

qt_add_resource(qmleasing "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)


#### Keys ignored in scope 1:.:.:qmleasing.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Easing" "Curve" "Editor"
