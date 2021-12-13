import QtQuick

Item {
    width: 480; height: 480

    Rectangle {
        id: viewport
        anchors.fill: parent
        anchors.margins: 100
        border.color: "red"

        TextEdit {
            font.pixelSize: 10
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
