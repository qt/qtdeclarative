import QtQuick

Window {
    id: root
    visible: true
    objectName: "root"
    width: 320
    height: 480

    property bool useTimer : false
    property int grabChangedCounter : 0

    Item {
        id: back
        anchors.fill: parent

        Rectangle {
            id: background
            anchors.fill: parent
            color: "blue"
        }

        Rectangle {
            id: container
            objectName: "container"
            anchors.fill: parent
            anchors.margins: 50
            z: 2

            Rectangle {
                id: likeButton
                color: "gray"
                anchors.centerIn: parent
                width: 200
                height: 200

                DragHandler {
                    id: handler
                    objectName: "dragHandler"
                    grabPermissions: PointerHandler.CanTakeOverFromItems
                    onGrabChanged: {
                        ++grabChangedCounter
                    }
                }
            }
        }

        Timer {
            id: reparentTimer
            running: false
            interval: 100
            repeat: false
            onTriggered: {
                container.parent = null
            }
        }

        Rectangle {
            id: likeButton2
            color: "yellow"
            anchors.centerIn: parent
            width: 100
            height: 100
            z: 3

            MultiPointTouchArea {
                id: press
                anchors.fill: parent
                onPressed: {
                    if (useTimer)
                        reparentTimer.running = true
                    else
                        container.parent = null
                }
            }
        }
    }
}
