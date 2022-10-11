import QtQuick

ListView {
    id: list
    snapMode: ListView.SnapOneItem
    model: 4
    width: 200
    height: 200
    highlightRangeMode: ListView.StrictlyEnforceRange
    highlight: Rectangle { width: 200; height: 200; color: "yellow" }
    delegate: Rectangle {
        id: wrapper
        width: list.width
        height: list.height
        Column {
            Text {
                text: index
            }
            Text {
                text: wrapper.x + ", " + wrapper.y
            }
        }
        color: ListView.isCurrentItem ? "lightsteelblue" : "transparent"
    }
    // speed up test runs
    flickDeceleration: 5000
    rebound: Transition {
        NumberAnimation {
            properties: "x,y"
            duration: 30
            easing.type: Easing.OutBounce
        }
    }
}
