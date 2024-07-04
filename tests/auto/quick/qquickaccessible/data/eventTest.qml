import QtQuick

Item {
    id: root
    Rectangle {
        id: button
        objectName: "button"
        color: "green"
        width: 100
        height: 50
        Accessible.role : Accessible.Button
        property string text : "test"

        Rectangle {
            objectName: "rect"
            anchors.fill: parent
            anchors.margins: 10
            color: "aqua"
            Text {
                objectName: "text"
                text : button.text
            }
        }
    }
}
