import QtQuick 2.0

Item {
    width: 320
    height: 480

    Rectangle {
        id: draggee
        width: 200
        height: 80
        x: 100
        y: 360
        gradient: Gradient {
            GradientStop { position: 0.0; color: "steelBlue" }
            GradientStop { position: 0.49; color: "white" }
            GradientStop { position: 0.5; color: "gray" }
            GradientStop { position: 1.0; color: "darkGray" }
        }
        radius: 20
        border.width: 2
        border.color: "black"
        Text {
            anchors.centerIn: parent
            font.pixelSize: 40
            text: "Position 2"
        }
    }

    ShaderEffectSource {
        id: source
        sourceItem: draggee
        hideSource: false
        property real margins: 6
        sourceRect: Qt.rect(-margins, -margins, sourceItem.width + 2 * margins, sourceItem.height + 2 * margins)
        smooth: true
    }

    ShaderEffect{
        id: effect
        anchors.fill: source.sourceItem
        anchors.margins: -source.margins
        property variant source: source
        property variant offset: Qt.size(4 / width, 4 / height)
        property variant delta: Qt.size(0.5 / width, 0.5 / height)

        fragmentShader: "qrc:shaders/shadow.frag"
    }
}
