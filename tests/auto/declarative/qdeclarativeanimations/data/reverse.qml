import QtQuick 2.0

Rectangle {
    width: 200
    height: 50

    Rectangle {
        id: myRectangle
        width: 50
        height: 50
        color: "green"
        x: 10
    }

    states: [
        State {
            name: "moved"
            PropertyChanges {
                target: myRectangle
                x: 100
            }
        }
    ]

    transitions: Transition { NumberAnimation { properties: "x" } }

    MouseArea {
        anchors.fill: parent
        onPressed: parent.state = "moved"
        onReleased: parent.state = ""
    }
}
