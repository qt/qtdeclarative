import QtQuick 2.15
import QtQml.Models 2.15
import Qt.treemodel

Rectangle {
    id: root
    width: 240
    height: 320
    color: "#ffffff"

    Component {
        id: myDelegate
        Rectangle {
            id: wrapper
            objectName: "wrapper"
            required property string display
            height: 20
            width: 240
            Text {
                objectName: "delegateText"
                text: display
            }
            color: ListView.isCurrentItem ? "lightsteelblue" : "white"
        }
    }

    DelegateModel {
        id: delegateModel
        objectName: "delegateModel"
        model: TreeModelCpp
        delegate: myDelegate
    }

    ListView {
        id: list
        objectName: "listView"
        model: delegateModel
        focus: true
        anchors.fill: parent
    }
}
