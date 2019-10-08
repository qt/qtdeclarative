import QtQuick 2.12
import QtQuick.Window 2.12

Window {
visible: true
width: 640
height: 480
title: qsTr("Hello World")

    Rectangle {
        id: colorRect
        objectName: "colorRect"
        anchors.fill: parent
        Text {
            objectName: "colorLabel"
            id: colorLabel
        }
    }

    Binding {
        target: colorLabel
        property: "text"
        value: "red"
    }
    Binding {
        target: colorRect
        property: "color"
        value: "red"
    }
}
