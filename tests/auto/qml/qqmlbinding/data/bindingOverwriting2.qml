pragma ComponentBehavior: Bound
import QtQuick

ListView {
    id: list
    property int i: 0

    model: 1
    delegate: Item {
        id: cellRootID
        required property int index
        Timer {
            interval: 1
            running: true
            onTriggered: {
                cellRootID.index = index + 123
                list.i = cellRootID.index
            }
        }
    }
}
