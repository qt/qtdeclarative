import QtQuick

Item {
    property alias firstRectangle: r1
    property alias secondRectangle: r2
    property alias thirdRectangle: r3
    Rectangle {
        id: r1
        anchors.fill: parent
        radius: 5
        topLeftRadius: 10
        bottomRightRadius: 2
    }
    Rectangle {
        id: r2
        anchors.fill: parent
        radius: 5
        topRightRadius: 10
        bottomLeftRadius: 2
    }
    Rectangle {
        id: r3
        anchors.fill: parent
        radius: 5
    }
}

