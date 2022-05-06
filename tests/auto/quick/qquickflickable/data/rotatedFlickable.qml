import QtQuick 2.0

Item {
    width: 300
    height: 300

    Flickable {
        id: flickable
        anchors.fill: parent
        contentHeight: column.height
        flickDeceleration: 5000 // speed up the test run

        Column {
            id: column
            width: parent.width

            Repeater {
                model: 255

                delegate: Rectangle {
                    width: column.width
                    height: 50
                    color: "gray"
                    Text {
                        anchors.centerIn: parent
                        text: index
                    }
                }
            }
        }
    }
}
