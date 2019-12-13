import QtQuick 2.0

Item {
    width: 320
    height: 480

    Rectangle {
        id: rect1
        x: 10
        y: 10
        width: 80
        height: 80
        radius: 20
        color: "black"
    }

    Rectangle {
        id: rect2
        x: 100
        y: 10
        width: 80
        height: 80
        radius: 20
        color: "black"
    }

    Rectangle {
        id: rect3
        x: 190
        y: 10
        width: 80
        height: 80
        radius: 20
        color: "black"
    }

    ShaderEffectSource {
        id: source
        property int counter
        sourceItem: rect1
        hideSource: true
    }

    ShaderEffect {
        id: effect
        anchors.fill: source.sourceItem

        property variant source: source

        fragmentShader: "qrc:shaders/gradient.frag"
    }


}
