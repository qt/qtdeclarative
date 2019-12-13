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
        width: parent.width
        height: parent.height / 2

        property variant source: source

        fragmentShader: "qrc:shaders/gradient3.frag"
    }

    ShaderEffect {
        width: parent.width
        y: parent.height / 2
        height: parent.height / 2

        property variant source: source

        fragmentShader: "qrc:shaders/gradient5.frag"
    }
}
