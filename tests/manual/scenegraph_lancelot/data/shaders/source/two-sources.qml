import QtQuick 2.0

Item {
    width: 320
    height: 480

    Rectangle {
        id: rect;
        anchors.centerIn: parent
        width: 1
        height: 10
        visible: false

        gradient: Gradient {
            GradientStop { position: 0; color: "#ff0000" }
            GradientStop { position: 0.5; color: "#00ff00" }
            GradientStop { position: 1; color: "#0000ff" }
        }
    }

    Text {
        id: text
        anchors.centerIn: parent
        font.pixelSize:  80
        text: "Shaderz!"
        visible: false
    }

    ShaderEffectSource {
        id: maskSource
        sourceItem: text
        smooth: true
    }

    ShaderEffectSource {
        id: colorSource
        sourceItem: rect;
        smooth: true
    }

    ShaderEffect {
        anchors.fill: text;

        property variant colorSource: colorSource
        property variant maskSource: maskSource;

        fragmentShader: "qrc:shaders/stencil.frag"
    }
}
