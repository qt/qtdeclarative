import QtQuick


Item {
    width: 640
    height: 480

    Column {
        anchors.top: parent.top
        spacing: 10
        Rectangle {
            objectName: "rect1"
            width: row1.width
            height: row1.height
            focusPolicy: Qt.TabFocus
            color: "red"
            border.width: activeFocus ? 2 : 0
            border.color: "black"
            Row {
                id: row1
                spacing: 10
                padding: 10
                Rectangle {
                    objectName: "rect11"
                    color: "yellow"
                    width: 30
                    height: 30
                    border.width: activeFocus ? 2 : 0
                    border.color: "black"
                    focusPolicy: Qt.TabFocus
                }
                Rectangle {
                    objectName: "rect12"
                    color: "yellow"
                    width: 30
                    height: 30
                    border.width: activeFocus ? 2 : 0
                    border.color: "black"
                    focusPolicy: Qt.TabFocus
                }
                Rectangle {
                    objectName: "rect13"
                    color: "yellow"
                    width: 30
                    height: 30
                    border.width: activeFocus ? 2 : 0
                    border.color: "black"
                    focusPolicy: Qt.TabFocus
                }
            }
        }
        Rectangle {
            objectName: "rect2"
            width: row2.width
            height: row2.height
            focusPolicy: Qt.TabFocus
            color: "red"
            border.width: activeFocus ? 2 : 0
            border.color: "black"
            Row {
                id: row2
                spacing: 10
                padding: 10
                Rectangle {
                    objectName: "rect21"
                    color: "yellow"
                    width: 30
                    height: 30
                    border.width: activeFocus ? 2 : 0
                    border.color: "black"
                    focusPolicy: Qt.TabFocus
                }
                Rectangle {
                    objectName: "rect22"
                    color: "yellow"
                    width: 30
                    height: 30
                    border.width: activeFocus ? 2 : 0
                    border.color: "black"
                    focusPolicy: Qt.TabFocus
                }
                Rectangle {
                    objectName: "rect23"
                    color: "yellow"
                    width: 30
                    height: 30
                    border.width: activeFocus ? 2 : 0
                    border.color: "black"
                    focusPolicy: Qt.TabFocus
                }
            }
        }
    }
}

