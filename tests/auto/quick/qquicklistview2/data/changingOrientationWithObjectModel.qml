import QtQuick

ListView {
    id: root

    readonly property bool isXReset: red.x === 0 && green.x === 0 && blue.x === 0 && cyan.x === 0 && magenta.x === 0 && teal.x === 0

    readonly property bool isYReset: red.y === 0 && green.y === 0 && blue.y === 0 && cyan.y === 0 && magenta.y === 0 && teal.y === 0

    width: 500
    height: 500
    model: ObjectModel {
          Rectangle {
            id: red
            width: root.width
            height: root.height
            color: "red"
        }
        Rectangle {
	    id: green
            width: root.width
            height: root.height
            color: "green"
        }
        Rectangle {
	    id: blue
            width: root.width
            height: root.height
            color: "blue"
        }
        Rectangle {
	    id: cyan
            width: root.width
            height: root.height
            color: "cyan"
        }
        Rectangle {
	    id: magenta
            width: root.width
            height: root.height
            color: "magenta"
        }
        Rectangle {
	    id: teal
            width: root.width
            height: root.height
            color: "teal"
        }
    }
    clip: true
    orientation: ListView.Vertical
    snapMode: ListView.SnapOneItem
    highlightRangeMode: ListView.StrictlyEnforceRange
}