import QtQuick 2.0

Rectangle {
    id: content

    property int contentHeight: height - senderText.implicitHeight + 2

    x: 1;
    width: 477
    height: Math.max(messageText.implicitHeight, 48) + senderText.implicitHeight + 6

    border.width: 1
    border.color:  "#404040"
    color: outbound ? "#202020" : "#313131"

    state: delegateState

    Item {
        id: avatarItem

        width: 48; height: 48

        anchors {
            left: outbound ? undefined : parent.left; right: outbound ? parent.right: undefined
            top: parent.top
            leftMargin: 3; topMargin: 3; rightMargin: 2
        }

        Image {
            id: avatarImage
            height: 48
            anchors.centerIn: parent
            sourceSize.width: 48

            source: avatar != "" ? avatar : "images/face-smile.png"
        }

        Component {
            id: sendComponent

            Rectangle {
                id: sendButton
                anchors { fill: parent; rightMargin: 1; bottomMargin: 1 }

                color:  "#202020"

                opacity: sendArea.pressed ? 0.5 : 1.0

                Behavior on opacity { NumberAnimation { duration: 150 } }

                Rectangle {
                    anchors.fill: parent
                    radius: 4
                    color: "#080808"
                }

                Text {
                    id: sendText
                    anchors.fill: parent
                    text: "Send"

                    color: "#FFFFFF"
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                MouseArea {
                    id: sendArea

                    anchors.fill: parent

                    onClicked:  root.send(editorLoader.item.text)
                }
            }
        }

        Loader {
            id: sendLoader
            anchors.fill: parent
        }
    }

    Text {
        id: messageText

        anchors {
            left: outbound ? parent.left : avatarItem.right; top: parent.top
            right: outbound ? avatarItem.left : parent.right; margins: 2
        }
        color: "#FFFFFF"
        font.pixelSize: 18
        wrapMode: Text.WordWrap
        text: message
    }

    Component {
        id: editorComponent

        TextEdit {
            color: "#FFFFFF"
            font.pixelSize: 18
            wrapMode: Text.WordWrap
            focus: true

            Keys.onReturnPressed: root.send(text)
            Keys.onEnterPressed: root.send(text)
        }
    }

    Loader {
        id: editorLoader
        anchors.fill: messageText
    }

    Text {
        id: senderText
        anchors { left: parent.left; bottom: parent.bottom; margins: 2 }
        color: "#DDDDDD"
        font.pixelSize: 12
        text: sender
    }

    Text {
        id: timeText
        anchors { right: parent.right; bottom: parent.bottom; margins: 2 }
        color: "#DDDDDD"
        font.pixelSize:  12
        text: time
    }

    states: State {
        name: "composing"

        PropertyChanges { target: editorLoader; sourceComponent: editorComponent }
        PropertyChanges { target: sendLoader; sourceComponent: sendComponent }
    }

    transitions: Transition {
        from: "composing"
        NumberAnimation { target: sendLoader.item; property: "opacity"; to: 0.0; duration: 3000 }
    }
}
