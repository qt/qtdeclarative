import QtQuick 2.0

Item
{
    id: root

    width: 100
    height: 100

    property bool layerEffect: false;
    onLayerEffectChanged: root.maybeUse();
    Component.onCompleted: root.maybeUse();

    property real layerOpacity: 1;
    property bool layerVisible: true;

    function maybeUse() {
        if (root.layerEffect)
            box.layer.effect = shaderEffect
    }

    Component {
        id: shaderEffect
        ShaderEffect {
            fragmentShader: "visible.frag.qsb"
        }

    }

    Rectangle {
        id: box
        width: 100
        height: 100

        color: "#0000ff"
        visible: parent.layerVisible;
        opacity: parent.layerOpacity;

        Rectangle {
            x: 50
            width: 50
            height: 100
            color: "#00ff00"
        }

        layer.enabled: true

    }
}
