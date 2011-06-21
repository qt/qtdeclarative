import QtQuick 2.0

Bubble {
    id: composerBubble

    property bool sending: false
    property int messageId
    property bool composing: true

    signal sent;

    outbound: true
    message: messageInput.text

    gradient: Gradient {
        GradientStop { position: 0.0; color: "#C1CDCD" }
        GradientStop {
            position: 1.0
            SequentialAnimation on position {
                running: sending
                alwaysRunToEnd: true
                loops: Animation.Infinite
                NumberAnimation { duration: 1000; from: 1.0; to: 0.1 }
                NumberAnimation { duration: 1000; from: 0.1; to: 1.0 }
            }
            color: "#F8F8FF"
        }
        GradientStop { position: 1.0; color: "#F8F8FF" }
    }

    Timer {
        interval: Math.random() * 20000
        running: composerBubble.sending
        onTriggered: {
            composerBubble.sending = false
            composerBubble.sent()
        }
    }

    TextInput {
        id: messageInput

        anchors.fill: parent

        focus: composing
        activeFocusOnPress: composing
        opacity: 0.0
        selectByMouse: false

        onAccepted: root.send()
    }

    onSent: messageModel.append({ "messageId": messageId, "outbound": outbound, "sender": sender, "message": message })
}
