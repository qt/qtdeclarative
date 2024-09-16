import QtQuick

Rectangle {
    width: 300
    height: 300

    FocusScope {
        width: parent.width
        height: parent.height
        focus: true

        Column {
            anchors.fill: parent
            anchors.rightMargin: 2
            anchors.leftMargin: 2
            anchors.topMargin: 10
            spacing: 20
            Rectangle {
                objectName: "rect1"
                width: parent.width
                height: 30
                border.width: 1
                border.color: activeFocus ? "blue" : "black"
                focusPolicy: Qt.TabFocus
            }
            Rectangle {
                objectName: "rect2"
                width: parent.width
                height: 30
                border.width: 1
                border.color: activeFocus ? "blue" : "black"
                focusPolicy: Qt.TabFocus
                focus: true
            }
        }
    }
}
