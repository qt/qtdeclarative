# Generated from nodetypes.pro.

#####################################################################
## nodetypes Binary:
#####################################################################

add_qt_executable(nodetypes
    GUI
    SOURCES
        nodetypes.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
add_qt_resource(nodetypes "nodetypes"
    PREFIX
        "/"
    FILES
        Animators.qml
        Effects.qml
        Images.qml
        Layers.qml
        LotsOfImages.qml
        LotsOfRects.qml
        Painter.qml
        Rects.qml
        Text.qml
        face-smile.png
        main.qml
        ps_shadow1.cso
        ps_shadow2.cso
        ps_wobble.cso
        qt.png
        shadow.png
        vs_wobble.cso
)


#### Keys ignored in scope 1:.:.:nodetypes.pro:<TRUE>:
# OTHER_FILES = "main.qml" "Rects.qml" "LotsOfRects.qml" "Images.qml" "LotsOfImages.qml" "Text.qml" "Animators.qml" "Layers.qml" "Effects.qml" "Painter.qml" "wobble.hlsl" "shadow1.hlsl" "shadow2.hlsl"
