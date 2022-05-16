import QtQuick
import QtQuick.Controls

Item {
    width: 500
    height: 500

    Drawer {
        y: 50
        height: 25
        visible: true
        edge: Qt.LeftEdge
        Label {
            text: "left"
        }
    }

    Drawer {
        y: 175
        height: 25
        visible: true
        enabled: false
        edge: Qt.LeftEdge
        Label {
            text: "left, disabled"
        }
    }

    Drawer {
        y: 300
        height: 25
        visible: true
        edge: Qt.LeftEdge
        dim: false
        Label {
            text: "left, no-dim"
        }
    }

    Drawer {
        y: 425
        height: 25
        visible: true
        edge: Qt.RightEdge
        Label {
            text: "right"
        }
    }
}