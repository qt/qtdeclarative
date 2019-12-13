import QtQuick 2.0

Item {
    width: 320
    height: 480

    ShaderEffectSource {
        id: source
        sourceItem: text
        textureSize: Qt.size(text.width / 2, text.height / 2)
        smooth: true
    }

    ShaderEffect {
        anchors.fill: text

        property variant source: source
        property variant textureSize: source.textureSize
        property color color: "black"

        fragmentShader: "qrc:shaders/edge.frag"
    }

    Text {
        id: text
        anchors.centerIn: parent
        font.pixelSize:  80
        text: "Shaderz!"
        color: "white"
    }
}
