import QtQuick 2.0

Item {
    width: 200
    height: 100

    Rectangle {
        id: box
        width: 200
        height: 100

        color: "#0000ff"

        Rectangle {
            x: 100
            width: 100
            height: 100
            color: "#00ff00"
        }

        visible: false

        layer.enabled: true
    }

    ShaderEffect {
        anchors.fill: parent
        property variant source: box
        fragmentShader: "textureProvider.frag.qsb"
    }
}
