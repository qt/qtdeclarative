import QtQuick

Item {
    id: root
    property alias childWidth: firstChild.width
    property alias childX: firstChild.x
    property alias childRotation: firstChild.rotation
    property double myrotation: 100
    property double myrotation2: 200
    height: 400
    y: 40

    Item {
        id: firstChild
        height: parent.height
        width: height
        y: parent.y
        x: y
        rotation: root.myrotation

        Item {
            id: inner
            anchors.fill: parent
        }
    }

    states: State {
        name: "reparented"
        ParentChange {
            target: firstChild
            parent: otherChild
            width: 2 *height
            x: 2 * y
            rotation: root.myrotation2
        }
    }

    Item {
        height: parent.height
        y: parent.y
        id: otherChild
    }
}
