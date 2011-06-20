import QtQuick 2.0

Rectangle {
    property string message
    property string sender
    property bool outbound: false

    width: 400
    height: messageText.implicitHeight + 4
    anchors {
        left: outbound ? parent.left : undefined
        right: outbound ? undefined : parent.right
    }

    radius: 6
    color: outbound ? "white" : "black"

    gradient: Gradient {
        GradientStop { position: 0.0; color: outbound ? "#FFFFFE" : "#696969" }
        GradientStop { position: 1.0; color: outbound ? "#FEF0C9" : "#708090" }
    }

    Text {
        id: messageText
        anchors { fill: parent; margins: 3 }
        color: outbound ? "black" : "white"
        font.pixelSize: 18
        wrapMode: Text.WordWrap
        text: sender + ": " + message
    }
}
