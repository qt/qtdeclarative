# Generated from pointer.pro.

#####################################################################
## pointer Binary:
#####################################################################

qt_internal_add_manual_test(pointer
    GUI
    SOURCES
        inputinspector.cpp inputinspector.h
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickPrivate
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
    "flickablesWithHandlers.qml"
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
    "resources/cursor-airbrush.png"
    "resources/cursor-eraser.png"
    "resources/cursor-felt-marker.png"
    "resources/cursor-pencil.png"
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
    "tabletCanvasDrawing.qml"
    "tapHandler.qml"
)

qt_internal_add_resource(pointer "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
set(quick_shared_resource_files
    "../../../examples/quick/shared/Button.qml"
    "../../../examples/quick/shared/CheckBox.qml"
    "../../../examples/quick/shared/Label.qml"
    "../../../examples/quick/shared/LauncherList.qml"
    "../../../examples/quick/shared/SimpleLauncherDelegate.qml"
    "../../../examples/quick/shared/Slider.qml"
    "../../../examples/quick/shared/TextField.qml"
    "../../../examples/quick/shared/images/back.png"
    "../../../examples/quick/shared/images/checkmark.png"
    "../../../examples/quick/shared/images/next.png"
    "../../../examples/quick/shared/images/slider_handle.png"
)

qt_internal_add_resource(pointer "quick_shared"
    PREFIX
        "/quick/shared"
    BASE
        "../../../examples/quick/shared"
    FILES
        ${quick_shared_resource_files}
)

qt_import_qml_plugins(pointer) # special case

#### Keys ignored in scope 1:.:.:pointer.pro:<TRUE>:
# TEMPLATE = "app"
