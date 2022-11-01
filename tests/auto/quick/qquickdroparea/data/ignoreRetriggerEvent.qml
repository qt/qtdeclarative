import QtQuick 2.0

DropArea {
    property int enterEvents: 0
    property int exitEvents: 0
    width: 100; height: 100
    objectName: "dropArea"
    onEntered: function (drag) { ++enterEvents; drag.accepted = false }
    onExited: {++exitEvents}
        Item {
        objectName: "dragItem"
        x: 50; y: 50
        width: 10; height: 10
    }
}
