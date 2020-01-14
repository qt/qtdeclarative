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

        Rectangle {
            width: 50
            height: 50
            color: "red"
            anchors.centerIn: parent
            transform: Rotation { angle: 45 }
        }
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
}
