import QtQuick 2.8
import QtQml.Models 2.1

DelegateModel {
    id: visualModel
    model: ListModel {
        id: myLM
        ListElement {
            name: "Apple"
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
            DelegateModel.inPersistedItems = true
            DelegateModel.inSelected = false
        }
        text: "item " + index
    }
}
