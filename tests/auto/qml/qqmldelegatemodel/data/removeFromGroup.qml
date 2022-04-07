import QtQuick 2.8
import QtQml.Models 2.1

Item {
    id: root
    width: 200
    height: 200

    DelegateModel {
        id: visualModel
        model: ListModel {
            id: myLM
            ListElement {
                name: "Apple"
            }
            ListElement {
                name: "Banana"
            }
            ListElement {
                name: "Orange"
            }
        }
        filterOnGroup: "selected"
        groups: [
            DelegateModelGroup {
                name: "selected"
                includeByDefault: true
            }
        ]
        delegate: Text {
            Component.onCompleted: {
                if (index === 1) {
                    DelegateModel.inSelected = false
                }
            }
            text: index + ": " + model.name
        }
    }

    // Needs an actual ListView in order for the DelegateModel to instantiate all items
    ListView {
        model: visualModel
        anchors.fill: parent
    }
}
