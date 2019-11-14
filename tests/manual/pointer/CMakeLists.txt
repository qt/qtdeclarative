# Generated from pointer.pro.

#####################################################################
## pointer Binary:
#####################################################################

qt_add_manual_test(pointer
    GUI
    SOURCES
        inputinspector.cpp inputinspector.h
        main.cpp
    LIBRARIES
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::Svg
)

# Resources:
set(qml_resource_files
    "content/CheckBox.qml"
    "content/FakeFlickable.qml"
    "content/FlashAnimation.qml"
    "content/MomentumAnimation.qml"
    "content/MouseAreaButton.qml"
    "content/MouseAreaSlider.qml"
    "content/MouseFeedbackSprite.qml"
    "content/MptaButton.qml"
    "content/MultiButton.qml"
    "content/ScrollBar.qml"
    "content/Slider.qml"
    "content/TapHandlerButton.qml"
    "content/TextBox.qml"
    "content/TouchpointFeedbackSprite.qml"
    "fakeFlickable.qml"
    "flickableWithHandlers.qml"
    "flingAnimation.qml"
    "joystick.qml"
    "main.qml"
    "map.qml"
    "map2.qml"
    "mixer.qml"
    "multibuttons.qml"
    "photosurface.qml"
    "pinchHandler.qml"
    "pointerDrag.qml"
    "resources/arrowhead.png"
    "resources/balloon.png"
    "resources/fighter.png"
    "resources/fingersprite.png"
    "resources/grabbing-location.svg"
    "resources/joystick-outer-case-pov.jpg"
    "resources/map.svgz"
    "resources/missile.png"
    "resources/mixer-knob.png"
    "resources/mouse.png"
    "resources/mouse_left.png"
    "resources/mouse_middle.png"
    "resources/mouse_right.png"
    "resources/redball.png"
    "sidebar.qml"
    "singlePointHandlerProperties.qml"
    "tapHandler.qml"
)

qt_add_resource(pointer "qml"
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

qt_add_resource(pointer "quick_shared"
    PREFIX
        "/quick/shared"
    BASE
        "../../../examples/quick/shared"
    FILES
        ${quick_shared_resource_files}
)


#### Keys ignored in scope 1:.:.:pointer.pro:<TRUE>:
# TEMPLATE = "app"
