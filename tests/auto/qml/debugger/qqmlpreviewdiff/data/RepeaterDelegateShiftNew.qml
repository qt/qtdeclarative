import QtQuick

Item {
    Item { objectName: "probe" }
    Repeater {
        model: 2
        delegate: Rectangle { objectName: "delegate" }
    }
}
