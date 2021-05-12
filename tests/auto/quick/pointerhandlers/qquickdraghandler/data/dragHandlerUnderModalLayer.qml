import QtQuick 2.15

import Test 1.0

Item {
    width: 640
    height: 480

    Rectangle {
        anchors.fill: parent
        color: "grey"

        Rectangle {
            x: 200
            y: 200
            width: 100
            height: 100
            color: "orange"
            DragHandler {
                grabPermissions: DragHandler.CanTakeOverFromAnything // but not anything with keepMouseGrab!
            }
        }
    }

    ModalLayer {
        anchors.fill: parent

        Rectangle {
            anchors.fill: parent
            color: "red"
            opacity: 0.4
        }
    }
}
