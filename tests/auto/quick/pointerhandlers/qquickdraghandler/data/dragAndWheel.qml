import QtQuick 2.15

Item {
    id: root

    property bool gotWheel: false
    property int changeCount: 0
    property alias wheelHandlerEnabled: wheelHandler.enabled

    width: 640
    height: 480

    Rectangle {
        color: "blue"
        width: 200
        height: 200

        DragHandler {
            id: dragHandler
        }

        WheelHandler {
            id: wheelHandler

            enabled: !dragHandler.active
            onEnabledChanged: root.changeCount++
            onWheel: root.gotWheel = true
        }

    }

}
