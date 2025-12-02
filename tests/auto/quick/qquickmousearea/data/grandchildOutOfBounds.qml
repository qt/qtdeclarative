import QtQuick

Item {
    objectName: "root"
    width: 240
    height: 240
    Rectangle {
        objectName: "parent rect"
        width: 200
        height: 100
        color: child.pressed ? "tomato" : "wheat"

        MouseArea {
            id: child
            objectName: "child MA"
            anchors.fill: parent
        }
        Rectangle {
            objectName: "child rect"
            x: 5
            y: 5
            width: 100
            height: 200
            color: grandchild.pressed ? "steelblue" : "aliceblue"

            MouseArea {
                id: grandchild
                objectName: "grandchild MA"
                anchors.fill: parent
            }
        }
    }
}
