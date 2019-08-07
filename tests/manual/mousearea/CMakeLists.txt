# Generated from mousearea.pro.

#####################################################################
## mousearea Binary:
#####################################################################

add_qt_executable(mousearea
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
add_qt_resource(mousearea "qml"
    PREFIX
        "/"
    FILES
        main.qml
        plainMouseArea.qml
)
set_source_files_properties("../../../examples/quick/shared/Button.qml"
    PROPERTIES alias "Button.qml"
)
set_source_files_properties("../../../examples/quick/shared/CheckBox.qml"
    PROPERTIES alias "CheckBox.qml"
)
set_source_files_properties("../../../examples/quick/shared/Label.qml"
    PROPERTIES alias "Label.qml"
)
set_source_files_properties("../../../examples/quick/shared/LauncherList.qml"
    PROPERTIES alias "LauncherList.qml"
)
set_source_files_properties("../../../examples/quick/shared/SimpleLauncherDelegate.qml"
    PROPERTIES alias "SimpleLauncherDelegate.qml"
)
set_source_files_properties("../../../examples/quick/shared/Slider.qml"
    PROPERTIES alias "Slider.qml"
)
set_source_files_properties("../../../examples/quick/shared/TextField.qml"
    PROPERTIES alias "TextField.qml"
)
set_source_files_properties("../../../examples/quick/shared/images/back.png"
    PROPERTIES alias "images/back.png"
)
set_source_files_properties("../../../examples/quick/shared/images/checkmark.png"
    PROPERTIES alias "images/checkmark.png"
)
set_source_files_properties("../../../examples/quick/shared/images/next.png"
    PROPERTIES alias "images/next.png"
)
set_source_files_properties("../../../examples/quick/shared/images/slider_handle.png"
    PROPERTIES alias "images/slider_handle.png"
)
add_qt_resource(mousearea "quick_shared"
    PREFIX
        "/quick/shared"
    BASE
        "../../../examples/quick/shared"
    FILES
        Button.qml
        CheckBox.qml
        Label.qml
        LauncherList.qml
        SimpleLauncherDelegate.qml
        Slider.qml
        TextField.qml
        images/back.png
        images/checkmark.png
        images/next.png
        images/slider_handle.png
)


#### Keys ignored in scope 1:.:.:mousearea.pro:<TRUE>:
# TEMPLATE = "app"
