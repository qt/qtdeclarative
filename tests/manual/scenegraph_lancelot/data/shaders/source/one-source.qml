import QtQuick 2.0

Item {
    width: 320
    height: 480

    Text {
        id: text
        anchors.centerIn: parent
        font.pixelSize:  80
        text: "Shaderz!"
        visible: false
    }

    ShaderEffectSource {
        id: source
        sourceItem: text
        smooth: true
    }

    ShaderEffect {
        anchors.fill: text;

        property variant source: source

        fragmentShader: "qrc:shaders/gradient.frag"
    }
}
