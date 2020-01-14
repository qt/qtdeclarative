import QtQuick 2.0

Item {
    width: 320
    height: 480

    Text {
        id: text
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        font.pixelSize: 80
        text: "Shaderz!"
    }

    ShaderEffectSource {
        id: source
        sourceItem: text
        hideSource: true
    }

    ShaderEffect {
        id: effect
        anchors.top: text.bottom
        anchors.left: text.left
        width: text.width
        height: text.height

        property variant source: source

        fragmentShader: "qrc:shaders/gradient.frag"
    }
}
