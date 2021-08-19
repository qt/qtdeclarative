import QtQuick

Item {
    width: 480; height: 480

    Rectangle {
        id: viewport
        anchors.fill: parent
        anchors.margins: 100
        border.color: "red"

        Text {
            Component.onCompleted: {
                for (let i = 0; i < 20; ++i)
                    text += "Line " + i + "\n";
            }
        }
    }
}
