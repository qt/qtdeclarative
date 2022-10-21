import QtQuick

Item {
    width: 480; height: 480

    Rectangle {
        id: viewport
        anchors.fill: parent
        anchors.margins: 100
        border.color: "red"

        FontLoader {
            id: ocr
            source: "tarzeau_ocr_a.ttf"
        }

        TextEdit {
            font.family: ocr.font.family
            font.styleName: ocr.font.styleName
            font.pixelSize: 15
            cursorDelegate: Rectangle {
                border.color: "green"
                border.width: 2
                color: "transparent"
                width: 10
            }
            Component.onCompleted: {
                for (let i = 0; i < 25; ++i)
                    text += "Line " + i + "\n";
            }
        }
    }
}
