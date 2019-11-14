# Generated from nodetypes.pro.

#####################################################################
## nodetypes Binary:
#####################################################################

add_qt_manual_test(nodetypes
    GUI
    SOURCES
        nodetypes.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
set(nodetypes_resource_files
    "Animators.qml"
    "Effects.qml"
    "Images.qml"
    "Layers.qml"
    "LotsOfImages.qml"
    "LotsOfRects.qml"
    "Painter.qml"
    "Rects.qml"
    "Text.qml"
    "face-smile.png"
    "main.qml"
    "ps_shadow1.cso"
    "ps_shadow2.cso"
    "ps_wobble.cso"
    "qt.png"
    "shadow.png"
    "vs_wobble.cso"
)

add_qt_resource(nodetypes "nodetypes"
    PREFIX
        "/"
    FILES
        ${nodetypes_resource_files}
)


#### Keys ignored in scope 1:.:.:nodetypes.pro:<TRUE>:
# OTHER_FILES = "main.qml" "Rects.qml" "LotsOfRects.qml" "Images.qml" "LotsOfImages.qml" "Text.qml" "Animators.qml" "Layers.qml" "Effects.qml" "Painter.qml" "wobble.hlsl" "shadow1.hlsl" "shadow2.hlsl"
