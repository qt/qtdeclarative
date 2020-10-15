import QtQuick 2.0

Item
{
    width: 200
    height: 100

    Rectangle {
        id: box
        width: 200
        height: 100

        color: "#ff0000"

        layer.enabled: true
        layer.sourceRect: Qt.rect(-10, -10, box.width + 20, box.height + 20);

        // A shader that pads the transparent pixels with blue.
        layer.effect: ShaderEffect {
            fragmentShader: "sourceRect.frag.qsb"
        }
    }
}
