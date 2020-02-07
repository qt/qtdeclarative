import QtQuick 2.12

Rectangle {
    width: 1280
    height: 720
    color: "#fdfdfd"

    Rectangle {
        id: rectangle7
        x: 387
        y: 90
        width: 630
        height: 505
        color: "#c2c2c2"

        Text {
            id: text1
            x: 148
            y: 224
            width: 334
            height: 58
            text: qsTrId("LID 014")
            font.pixelSize: 30
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.bold: true
        }

        MultiTextItem {
            id: multiTextItem
            x: 252
            y: 288
            width: 135
            height: 34
            currentIndex: 0

            MultiTextElement {
                id: multiTextElement
                text: qsTrId("LID 021")
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            MultiTextElement {
                id: multiTextElement1
                text: qsTrId("LID 022")
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    Column {
        id: column
        x: 0
        y: 85
        width: 381
        height: 570
        clip: false

        Rectangle {
            id: rectangle3
            width: column.width
            height: 114
            color: "#efeff1"
            border.width: 1
        }

        Rectangle {
            id: rectangle4
            width: column.width
            height: 114
            color: "#efeff1"
            border.width: 1
        }

        Rectangle {
            id: rectangle5
            width: column.width
            height: 114
            color: "#efeff1"
            border.width: 1
        }
    }

    Rectangle {
        id: rectangle
        x: 0
        y: 0
        width: 1280
        height: 84
        color: "#c2c2c2"
        border.width: 1

        TextInput {
            id: textInput
            x: 17
            y: 21
            width: 539
            height: 43
            text: qsTrId("LID 001")
            font.pixelSize: 12
        }
    }

    Item {
        x: 1032
        y: 90
        width: 221
        height: 505

        Column {
            id: column1
            x: 0
            y: 40
            width: 197
            height: 360
            spacing: 50

            TextEdit {
                id: textEdit
                width: 80
                height: 20
                text: qsTrId("LID 019")
                font.pixelSize: 12
            }
        }
    }
}
