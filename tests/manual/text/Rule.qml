import QtQuick

Text {
    id: root
    horizontalAlignment: Text.AlignHCenter
    Rectangle {
        id: leftRule
        width: (root.width - root.contentWidth) / 2 - 5
        height: 4
        y: root.height / 2
        color: "steelblue"
    }
    Rectangle {
        width: leftRule.width
        height: 4
        x: root.width - width
        y: root.height / 2
        color: "steelblue"
    }
}
