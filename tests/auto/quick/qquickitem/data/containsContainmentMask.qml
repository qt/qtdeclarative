import QtQuick 2.11

Item {
    width: 200
    height: 200

    Item {
        id: firstItem

        objectName: "firstItem"
        x: 6
        y: 6
        width: 1000
        height: 1000

        containmentMask: Item {
            x: -5
            y: -5
            width: 10
            height: 10
        }
    }

    Item {
        id: secondItem

        objectName: "secondItem"
        x: 6
        y: 6
        width: 0
        height: 0

        containmentMask: Item {
            parent: secondItem
            anchors.centerIn: parent
            width: 10
            height: 10
        }
    }
}
