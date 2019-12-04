import QtQuick 2.14
import QtQml.Models 2.14

Item {
    id: root
    visible: true
    width: 640
    height: 480

    property alias tableView: tv

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
        property int modelId: 0

        model: {
            switch (modelId) {
            case 0:  return lm;
            case 1:  return om;
            case 2:  return dm;
            default: return null;
            }
        }

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
