import QtQuick 2.0

Flickable {
    objectName: "outerFlickable"
    property bool pressed: ma.pressed
    width: 240
    height: 320
    contentWidth: 480
    contentHeight: 320
    flickableDirection: Flickable.HorizontalFlick
    pressDelay: 10000

    // faster rebound to speed up test runs
    rebound: Transition {
        NumberAnimation {
            properties: "x,y"
            duration: 30
            easing.type: Easing.OutBounce
        }
    }

    MouseArea {
        objectName: "filteringMouseArea"
        x: 20
        y: 20
        width: 400
        height: 300
        drag.filterChildren: true
        Rectangle {
            id: rectangle
            color: "yellow"
            anchors.fill: parent
        }
        Flickable {
            objectName: "innerFlickable"
            anchors.fill: parent
            flickableDirection: Flickable.HorizontalFlick
            contentWidth: 1480
            contentHeight: 400
            pressDelay: 50

            // faster rebound to speed up test runs
            rebound: Transition {
                NumberAnimation {
                    properties: "x,y"
                    duration: 30
                    easing.type: Easing.OutBounce
                }
            }

            Rectangle {
                y: 100
                x: 80
                width: 240
                height: 100
                color: ma.pressed ? 'blue' : 'green'
                MouseArea {
                    id: ma
                    objectName: "mouseArea"
                    anchors.fill: parent
                }
            }
        }
    }
}

