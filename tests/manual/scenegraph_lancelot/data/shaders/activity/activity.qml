import QtQuick 2.0

Item {
    width: 320
    height: 480

    Text {
        id: text
        anchors.centerIn: parent
        font.pixelSize:  80
        text: "Shaderz!"
    }

    ShaderEffectSource {
        id: source
        sourceItem: text
        hideSource: effect.visible
    }

    ShaderEffect {
        id: effect
        anchors.fill: text

        property variant source: source

        fragmentShader: "qrc:shaders/gradient.frag"
    }
}
