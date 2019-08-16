
import QtQuick 2.0

Item {
    width: 300
    height:200
    property var clicks: []
    
    Rectangle {
        x: 75
        y: 75
        width: 200
        height: 100
        color: "salmon"

        MouseArea {
            objectName: "parentMouseArea"
            anchors.fill: parent
            onClicked: clicks.push(objectName)
        }
        
        Rectangle {
            x: 25
            y: 25
            width: 200
            height: 100
            color: "lightsteelblue"

            MouseArea {
                id: mouseArea
                objectName: "childMouseArea"
                anchors.fill: parent
                onClicked: clicks.push(objectName)
            }
        }
    }
}
