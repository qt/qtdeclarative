import QtQuick
import QtQuick.Controls

Item {
    width: 500
    height: 500

    Drawer {
        x: 50
        width: 25
        height: txt0.implicitWidth
        visible: true
        edge: Qt.TopEdge
        Item {
            width: txt0.implicitHeight
            height: txt0.implicitWidth
            Label {
                id: txt0
                anchors.centerIn: parent
                rotation: 90
                text: "top"
            }
        }
    }

    Drawer {
        x: 175
        width: 25
        height: txt1.implicitWidth
        visible: true
        enabled: false
        edge: Qt.TopEdge
        Item {
            width: txt1.implicitHeight
            height: txt1.implicitWidth
            Label {
                id: txt1
                anchors.centerIn: parent
                rotation: 90
                text: "top, disabled"
            }
        }
    }

    Drawer {
        x: 300
        width: 25
        height: txt2.implicitWidth
        visible: true
        edge: Qt.TopEdge
        dim: false
        Item {
            width: txt2.implicitHeight
            height: txt2.implicitWidth
            Label {
                id: txt2
                anchors.centerIn: parent
                rotation: 90
                text: "top, no-dim"
            }
        }
    }

    Drawer {
        x: 425
        width: 25
        height: txt3.implicitWidth
        visible: true
        edge: Qt.BottomEdge
        Item {
            width: txt3.implicitHeight
            height: txt3.implicitWidth
            Label {
                id: txt3
                anchors.centerIn: parent
                rotation: 90
                text: "bottom"
            }
        }
    }
}