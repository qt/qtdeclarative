import QtQuick 2.6

Text {
    width: 200; height: 200
    text: "Hello Qt"

    padding: 10
    topPadding: 20
    leftPadding: 30
    rightPadding: 40
    bottomPadding: 50

    Rectangle {
        width: parent.leftPadding
        height: parent.height
        color: "#6600FF00"
    }

    Rectangle {
        width: parent.width
        height: parent.topPadding
        color: "#66888800"
    }

    Rectangle {
        x: parent.width - parent.rightPadding
        width: parent.rightPadding
        height: parent.height
        color: "#6600FFFF"
    }

    Rectangle {
        y: parent.height - parent.bottomPadding
        width: parent.width
        height: parent.bottomPadding
        color: "#66880088"
    }
}
