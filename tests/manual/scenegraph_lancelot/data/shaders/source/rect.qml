import QtQuick 2.0

Item {
    width: 320
    height: 480

    Text {
        id: text
        anchors.centerIn: parent
        font.pixelSize:  80
        text: "Shaderz\nroolz!"
        horizontalAlignment: Text.AlignHCenter
    }

    ListModel {
        id: rects
        ListElement {x0: 50;  y0: 50;  w: 100; h: 100}
        ListElement {x0: 50;  y0: 200; w: 100; h: 100}
        ListElement {x0: 180; y0: 50;  w: 150; h: 380}
    }

    Repeater {
        model: rects

        ShaderEffect {
            x: x0
            y: y0
            width: w
            height: h
            property variant source: ShaderEffectSource {
                sourceItem: text
                sourceRect: Qt.rect(x0 - text.x, y0 - text.y, w, h)
            }
            fragmentShader: "qrc:shaders/gradient.frag.qsb"
        }
    }
}
