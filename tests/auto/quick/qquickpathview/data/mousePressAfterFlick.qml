import QtQuick

Item {
    id: root
    width: 360
    height: 900

    MouseArea {
        id: rootMouseArea
        objectName: "rootMouseArea"
        anchors.fill: parent
    }

    PathView {
        id: pathView
        objectName: "pathView"
        anchors.centerIn: parent
        width: 150
        height: parent.height
        dragMargin: Number.POSITIVE_INFINITY

        preferredHighlightBegin: 0.5
        preferredHighlightEnd: 0.5
        pathItemCount: 9

        Item {
            anchors.fill: parent
        }

        model: 10

        delegate: Rectangle {
            id: delegate
            width: parent.width
            height: 120
            color: "red"
            Text {
                anchors.fill: parent
                horizontalAlignment: Qt.AlignHCenter
                text: "Path element " + modelData
            }
        }

        path: Path {
            id: path
            property int width: pathView.width
            startX: path.width / 2
            startY: 0

            PathCurve {
                x: path.width / 2
                relativeY: 32
            }
            PathCurve {
                x: path.width / 2
                relativeY: pathView.height - 32
            }
        }
    }
}

