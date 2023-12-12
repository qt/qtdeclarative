import QtQuick

Item {
    Component.onCompleted: item.focus = true
    width: 640
    height: 480

    Column {
        anchors.top: parent.top
        anchors.topMargin: 10
        spacing: 10
        objectName: "column"
        focusPolicy: Qt.ClickFocus

        Item {
            id: item
            implicitWidth: 100
            implicitHeight: 20
            objectName: "item"
            focusPolicy: Qt.TabFocus

            Rectangle {
                id: rect
                anchors.fill: parent
                color: "yellow"
                opacity: 0.5
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: function onClicked(mouseEvent) {
                    if (mouseEvent.button == Qt.RightButton)
                        rect.color = "pink"
                }
            }
        }

        Item {
            id: customText
            objectName: "customText"
            implicitWidth: 100
            implicitHeight: 50
            TextInput {
                anchors.fill: parent
                objectName: "textInputChild"
                text: parent.activeFocus ? "focus" : "no focus"
            }
            activeFocusOnTab: true
        }

        Item {
            id: customItem
            objectName: "customItem"
            implicitWidth: 100
            implicitHeight: 50
            Rectangle {
                anchors.fill: parent
                color: parent.activeFocus ? "red" : "blue"
                opacity: 0.3
            }
            focusPolicy: Qt.WheelFocus
        }

        Text {
            id: hyperlink
            objectName: "hyperlink"
            color: "blue"
            onLinkActivated: { text = "Clicked"; }
            textFormat: Text.RichText
            text: "<a href=\"http://qt-project.org\">Qt Project website</a>"
            focusPolicy: Qt.StrongFocus

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                acceptedButtons: Qt.NoButton // Don't eat the mouse clicks
                cursorShape: Qt.PointingHandCursor
                // the acceptedButtons will take precedence
                // and the click focus policy will be ignored
                focusPolicy: Qt.ClickFocus
            }
        }
    }
}
