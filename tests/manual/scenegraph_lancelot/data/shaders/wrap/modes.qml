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
        id: clamp
        sourceItem: text
        smooth: true
        wrapMode: ShaderEffectSource.ClampToEdge
    }

    ShaderEffectSource {
        id: hRepeat
        sourceItem: text
        smooth: true
        wrapMode: ShaderEffectSource.RepeatHorizontally
    }

    ShaderEffectSource {
        id: vRepeat
        sourceItem: text
        smooth: true
        wrapMode: ShaderEffectSource.RepeatVertically
    }

    ShaderEffectSource {
        id: repeat
        sourceItem: text
        smooth: true
        wrapMode: ShaderEffectSource.Repeat
    }

    ShaderEffect {
        anchors.fill: parent

        property variant cyan: hRepeat
        property variant magenta: vRepeat
        property variant yellow: repeat
        property variant black: clamp

        fragmentShader: "qrc:shaders/cmyk.frag"
    }
}
