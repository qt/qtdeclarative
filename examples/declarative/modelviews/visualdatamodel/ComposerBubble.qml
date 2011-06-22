import QtQuick 2.0

Rectangle {
    id: composer

    property bool sending: false
    property bool sent: false
    property int margin: senderText.height + 1
    property int messageId;
    property string sender
    property url avatar

    function send() {
        timeText.text = Qt.formatTime(Date.now())
        sending = true
        sendText.visible = false
        sendArea.enabled = false
        messageText.focus = false
        messageText.activeFocusOnPress = false

        root.send()
    }

    border.width: 1
    border.color:  "#404040"
    color: "#202020"

    x: 1
    width:  477
    height: Math.max(messageText.implicitHeight, 48) + senderText.implicitHeight + 6

    Behavior on height {
        NumberAnimation { duration: 500 }
    }

    Timer {
        interval: Math.random() * 20000
        running: composer.sending
        onTriggered: {
            composer.sending = false
            composer.sent = true
            messageModel.append({
                "messageId": messageId,
                "sender": sender,
                "message": messageText.text,
                "time": timeText.text,
                "outbound": true,
                "avatar": avatar
            })
        }
    }

    Item {
        id: avatarItem

        width: 48; height: 48
        anchors { right: parent.right; top: parent.top; topMargin: 3; rightMargin: 2 }
        Image {
            id: avatarImage
            height: 48
            anchors.centerIn: parent
            sourceSize.width: 48

            source: avatar != "" ? avatar : "images/face-smile.png"

            SequentialAnimation on opacity {
                loops: Animation.Infinite
                running:  composer.sending
                alwaysRunToEnd: true
                NumberAnimation {
                    from: 1.0; to: 0.0
                    duration: 5000
                }
                NumberAnimation {
                    from: 0.0; to: 1.0
                    duration: 5000
                }
            }

        }
        Rectangle {
            anchors { fill: parent; rightMargin: 1; bottomMargin: 1 }

            color:  "#000000"

            opacity: sendArea.pressed ? 0.5 : (composer.sending || composer.sent ? 0.0 : 1.0)

            Behavior on opacity { NumberAnimation { duration: 150 } }

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

                onClicked:  composer.send()
            }
        }
    }

    TextEdit {
        id: messageText

        anchors { left: parent.left; top: parent.top; right: avatarItem.left; margins: 2 }
        color: "#FFFFFF"
        font.pixelSize: 18
        wrapMode: Text.WordWrap
        focus: true

        Keys.onReturnPressed: composer.send()
        Keys.onEnterPressed: composer.send()
    }

    Text {
        id: senderText
        anchors { left: parent.left; bottom: parent.bottom; margins: 2 }
        color: "#DDDDDD"
        font.pixelSize: 12
        text: root.sender
    }

    Text {
        id: timeText
        anchors { right: parent.right; bottom: parent.bottom; margins: 2 }
        color: "#DDDDDD"
        font.pixelSize:  12
    }
}
