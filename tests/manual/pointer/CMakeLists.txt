# Generated from pointer.pro.

#####################################################################
## pointer Binary:
#####################################################################

add_qt_executable(pointer
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
add_qt_resource(pointer "qml"
    PREFIX
        "/"
    FILES
        content/CheckBox.qml
        content/FakeFlickable.qml
        content/FlashAnimation.qml
        content/MomentumAnimation.qml
        content/MouseAreaButton.qml
        content/MouseAreaSlider.qml
        content/MouseFeedbackSprite.qml
        content/MptaButton.qml
        content/MultiButton.qml
        content/ScrollBar.qml
        content/Slider.qml
        content/TapHandlerButton.qml
        content/TextBox.qml
        content/TouchpointFeedbackSprite.qml
        fakeFlickable.qml
        flickableWithHandlers.qml
        flingAnimation.qml
        joystick.qml
        main.qml
        map.qml
        map2.qml
        mixer.qml
        multibuttons.qml
        photosurface.qml
        pinchHandler.qml
        pointerDrag.qml
        resources/arrowhead.png
        resources/balloon.png
        resources/fighter.png
        resources/fingersprite.png
        resources/grabbing-location.svg
        resources/joystick-outer-case-pov.jpg
        resources/map.svgz
        resources/missile.png
        resources/mixer-knob.png
        resources/mouse.png
        resources/mouse_left.png
        resources/mouse_middle.png
        resources/mouse_right.png
        resources/redball.png
        sidebar.qml
        singlePointHandlerProperties.qml
        tapHandler.qml
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
add_qt_resource(pointer "quick_shared"
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


#### Keys ignored in scope 1:.:.:pointer.pro:<TRUE>:
# TEMPLATE = "app"
