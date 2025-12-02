import QtQuick 2.0

Item{
    objectName: "declared root item"
    width: 400
    height: 200
    property bool point1: ma2.containsMouse && !ma1.containsMouse
    property bool point2: ma3.containsMouse && ma4.containsMouse
    Rectangle{
        objectName: "left rect"
        width: 200
        height: 200
        color: ma1.containsMouse ? "red" : "white"
        MouseArea{
            id: ma1
            hoverEnabled: true
            anchors.fill: parent
        }
        Rectangle{
            objectName: "left child rect"
            width: 100
            height: 100
            color: ma2.containsMouse ? "blue" : "white"
            MouseArea{
                id: ma2
                hoverEnabled: true
                anchors.fill: parent
            }
        }
    }

    Item{
        x:200
        objectName: "right item"
        Rectangle{
            objectName: "right rect"
            width: 200
            height: 200
            color: ma3.containsMouse ? "yellow" : "white"
            Rectangle{
                objectName: "right child rect"
                width: 100
                height: 100
                color: ma4.containsMouse ? "green" : "white"
            }
        }
        MouseArea{
            id: ma3
            hoverEnabled: true
            width: 200
            height: 200
            MouseArea{
                id: ma4
                width: 100
                height: 100
                hoverEnabled: true
            }
        }
    }
}
