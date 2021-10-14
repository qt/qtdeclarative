import QtQuick

Item {
    id: root
    property alias childWidth: firstChild.width
    property alias childRotation: firstChild.rotation
    property double myrotation: 100
    property double myrotation2: 200
    height: 400

    Item {
        id: firstChild
        height: parent.height
        width: height
        rotation: root.myrotation
    }

    states: State {
        name: "reparented"
        ParentChange { target: firstChild; parent: otherChild; width: 2*height; rotation:  root.myrotation2}
    }

    Item {
        height: parent.height
        id: otherChild
    }
}
