import QtQuick
import QtQuick.Window
import QtQml.Models

Window {
    id: win
    visible: true
    width: 640
    height: 480
    property int destroyCount : 0;
    property int createCount : 0;
    property alias testListModel: mdl

    DelegateModel {
        id: visualModel
        model: ListModel {
            id: mdl
            ListElement {
                name: "a"
                hidden: false
            }
            ListElement {
                name: "b"
                hidden: true
            }
            ListElement {
                name: "c"
                hidden: false
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
            visible: DelegateModel.inSelected
            property var idx
            Component.onCompleted: {
                ++createCount
                idx = index
                DelegateModel.inPersistedItems = true
                DelegateModel.inSelected = !model.hidden
            }
            Component.onDestruction: ++destroyCount
            text: model.name
        }
    }

    ListView {
    id: listView
        model: visualModel
        anchors.fill: parent
        focus: true
    }

}
