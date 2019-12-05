import QtQuick 2.14
import QtQml.Models 2.14

Item {
    id: root
    visible: true
    width: 640
    height: 480

    property alias tableView: tv
    property alias objectModel: om
    property alias listModel: lm
    property alias delegateModel: dm

    ObjectModel {
        id: om
        Rectangle { implicitHeight: 30; implicitWidth: 80; color: "red" }
        Rectangle { implicitHeight: 30; implicitWidth: 80; color: "green" }
        Rectangle { implicitHeight: 30; implicitWidth: 80; color: "blue" }
    }

    ListModel {
        id: lm
        ListElement { name: "1" }
        ListElement { name: "44"}
    }

    DelegateModel {
       id: dm
       model: ListModel {
           ListElement { name: "Apple" }
           ListElement { name: "Orange" }
       }
       delegate: Rectangle {
           implicitHeight: 25
           implicitWidth: 100
           Text { text: "Name: " + name}
       }
    }
    TableView {
        id: tv
        visible: true
        anchors.fill: parent

        delegate: Rectangle {
            id: dlg
            implicitWidth: 40
            implicitHeight: 20
            color: "red"
            Text {
                text: qsTr("name: " + name)
            }
            border.color: "green"
        }
    }
}
