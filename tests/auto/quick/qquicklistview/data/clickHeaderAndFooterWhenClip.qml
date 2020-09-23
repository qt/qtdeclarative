import QtQuick 2.9
import QtQuick.Window 2.15

ListView {
    id: list
    anchors.fill: parent
    property bool headerPressed: false
    property bool footerPressed: false
    model: ListModel {
        ListElement {
            name: "Element 1"
        }
        ListElement {
            name: "Element 2"
        }
        ListElement {
            name: "Element 3"
        }
        ListElement {
            name: "Element 4"
        }
        ListElement {
            name: "Element 5"
        }
        ListElement {
            name: "Element 6"
        }
    }
    clip: true
    headerPositioning: ListView.OverlayHeader
    footerPositioning: ListView.OverlayHeader

    delegate: Text {
        height: 100
        text: name
    }

    header: Rectangle {
        width: parent.width
        height: 50
        z: 2
        color: "blue"
        MouseArea {
            objectName: "header"
            anchors.fill: parent
            onClicked: list.headerPressed = true
        }
    }
    footer: Rectangle {
        width: parent.width
        height: 50
        color: "red"
        z: 2
        MouseArea {
            objectName: "footer"
            anchors.fill: parent
            onClicked: list.footerPressed = true
        }
    }
}
