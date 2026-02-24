import QtQuick

Rectangle {
    id: rect

    Timer {
        running: true
        interval: 1

        onTriggered: {
            rectRotation.axis.x = .2;
            rectRotation.axis.y = .4;
            rectRotation.axis.z = .8;
        }
    }


    transform: Rotation {
        id: rectRotation
        axis {
            x: 1
            y: 0
            z: 0

            Behavior on x { NumberAnimation { duration: 100 } }
            Behavior on y { NumberAnimation { duration: 100 } }
            Behavior on z { NumberAnimation { duration: 100 } }
        }
    }
}
