import QtQuick

ListView {
    id: root

    function allDelegates(valueSelector) {
        let sum = 0;
        for (let i = 0; i < root.count; i++)
            sum += valueSelector(root.itemAtIndex(i));
	return sum;
    }

    readonly property bool isXReset: allDelegates(function(item) { return item?.x ?? 0; }) === 0
    readonly property bool isYReset: allDelegates(function(item) { return item?.y ?? 0; }) === 0

    width: 500
    height: 500
    delegate: Rectangle {
        width: root.width
        height: root.height
        color: c
    }
    model: ListModel {
        ListElement {
            c: "red"
        }
        ListElement {
            c: "green"
        }
        ListElement {
            c: "blue"
        }
        ListElement {
            c: "cyan"
        }
        ListElement {
            c: "magenta"
        }
        ListElement {
            c: "teal"
        }
    }
    clip: true
    orientation: ListView.Vertical
    snapMode: ListView.SnapOneItem
    highlightRangeMode: ListView.StrictlyEnforceRange
}