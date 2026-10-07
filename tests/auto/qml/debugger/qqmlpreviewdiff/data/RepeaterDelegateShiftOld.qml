import QtQuick

Item {
    Repeater {
        model: 2
        delegate: Rectangle { objectName: "delegate" }
    }
}
