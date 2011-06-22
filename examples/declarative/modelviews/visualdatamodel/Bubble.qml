import QtQuick 2.0

Rectangle {
    x: 1
    width:  477
    height: Math.max(messageText.implicitHeight, 48) + senderText.implicitHeight + 6

    border.width: 1
    border.color:  "#404040"
    color: outbound ? "#202020" : "#313131"

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
}
