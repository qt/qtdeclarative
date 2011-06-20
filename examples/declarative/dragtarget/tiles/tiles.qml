import QtQuick 2.0

Rectangle {
    id: root

    width: 620
    height: 410

    color: "black"

    DragTarget {
        id: resetTarget

        anchors.fill: parent
    }

    Grid {
        id: redDestination

        anchors.left: redSource.right; anchors.top: parent.top;
        anchors.margins: 5
        width: 300
        height: 300

        opacity: 0.5

        columns: 3

        Repeater {
            model: 9
            delegate: DropTile {
                colorKey: "red"
            }
        }
    }

    Grid {
        id: blueDestination

        anchors.right: blueSource.left; anchors.bottom: parent.bottom;
        anchors.margins: 5
        width: 300
        height: 300

        opacity: 0.5

        columns: 3

        Repeater {
            model: 9
            delegate: DropTile {
                colorKey: "blue"
            }
        }
    }

    Column {
        id: redSource

        anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
        anchors.margins: 5
        width: 100

        Repeater {
            model: 9
            delegate: DragTile {
                colorKey: "red"
            }
        }
    }
    Column {
        id: blueSource

        anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
        anchors.margins: 5
        width: 100

        Repeater {
            model: 9
            delegate: DragTile {
                colorKey: "blue"
            }
        }
    }
}
