# Generated from text.pro.

#####################################################################
## text Binary:
#####################################################################

qt_add_manual_test(text
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
set(qml_resource_files
    "SignalIndicator.qml"
    "main.qml"
    "textInputPropertiesAndSignals.qml"
)

qt_add_resource(text "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
set_source_files_properties("../../../examples/quick/shared/Button.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Button.qml"
)
set_source_files_properties("../../../examples/quick/shared/CheckBox.qml"
    PROPERTIES QT_RESOURCE_ALIAS "CheckBox.qml"
)
set_source_files_properties("../../../examples/quick/shared/Label.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Label.qml"
)
set_source_files_properties("../../../examples/quick/shared/LauncherList.qml"
    PROPERTIES QT_RESOURCE_ALIAS "LauncherList.qml"
)
set_source_files_properties("../../../examples/quick/shared/SimpleLauncherDelegate.qml"
    PROPERTIES QT_RESOURCE_ALIAS "SimpleLauncherDelegate.qml"
)
set_source_files_properties("../../../examples/quick/shared/Slider.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Slider.qml"
)
set_source_files_properties("../../../examples/quick/shared/TextField.qml"
    PROPERTIES QT_RESOURCE_ALIAS "TextField.qml"
)
set_source_files_properties("../../../examples/quick/shared/images/back.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/back.png"
)
set_source_files_properties("../../../examples/quick/shared/images/checkmark.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/checkmark.png"
)
set_source_files_properties("../../../examples/quick/shared/images/next.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/next.png"
)
set_source_files_properties("../../../examples/quick/shared/images/slider_handle.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/slider_handle.png"
)
set(quick_shared_resource_files
    "Button.qml"
    "CheckBox.qml"
    "Label.qml"
    "LauncherList.qml"
    "SimpleLauncherDelegate.qml"
    "Slider.qml"
    "TextField.qml"
    "images/back.png"
    "images/checkmark.png"
    "images/next.png"
    "images/slider_handle.png"
)

qt_add_resource(text "quick_shared"
    PREFIX
        "/quick/shared"
    BASE
        "../../../examples/quick/shared"
    FILES
        ${quick_shared_resource_files}
)


#### Keys ignored in scope 1:.:.:text.pro:<TRUE>:
# TEMPLATE = "app"
