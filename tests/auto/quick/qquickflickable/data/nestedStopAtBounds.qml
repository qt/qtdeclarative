import QtQuick 2.0

Flickable {
    id: outer
    objectName: "outerFlickable"
    width: 400
    height: 400
    contentX: 50
    contentY: 50
    contentWidth: 500
    contentHeight: 500
    flickableDirection: inner.flickableDirection
    Text { x: 100; y: 80; text: "dragging: outer " + outer.dragging + " inner " + inner.dragging }

    // faster rebound to speed up test runs
    rebound: Transition {
        NumberAnimation {
            properties: "x,y"
            duration: 30
            easing.type: Easing.OutBounce
        }
    }

    Rectangle {
        x: 100
        y: 100
        width: 300
        height: 300

        color: "yellow"
        objectName: "yellowRect"
        Text {
            text: "...."
            y: 250
        }

        Flickable {
            id: inner
            objectName: "innerFlickable"
            anchors.fill: parent
            contentX: 100
            contentY: 100
            contentWidth: 400
            contentHeight: 400
            boundsBehavior: Flickable.StopAtBounds

            // faster rebound to speed up test runs
            rebound: Transition {
                NumberAnimation {
                    properties: "x,y"
                    duration: 30
                    easing.type: Easing.OutBounce
                }
            }

            Rectangle {
                anchors.fill: parent
                anchors.margins: 100
                objectName: "blueRect"
                color: "blue"
            }
            MouseArea {
                anchors.fill: parent
                objectName: "mouseArea"
            }
        }
    }
}
