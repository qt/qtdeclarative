import QtQml

DelegateModel {
    id: root

    property ListModel listModel: ListModel {
        ListElement {
            modelData: "a"
            row: "b"
            column: "c"
            model: "d"
            hasModelChildren: "e"
            index: "f"
        }
    }

    property var array: [{
        modelData: "a",
        row: "b",
        column: "c",
        model: "d",
        hasModelChildren: "e",
        index: "f"
    }]

    property QtObject object: QtObject {
        property string modelData: "a"
        property string row: "b"
        property string column: "c"
        property string model: "d"
        property string hasModelChildren: "e"
        property string index: "f"
    }

    property int n: -1

    model: {
        switch (n) {
        case 0: return listModel
        case 1: return array
        case 2: return object
        }
        return undefined;
    }

    delegate: QtObject {
        required property string modelData
        required property string row
        required property string column
        required property string model
        required property string hasModelChildren
        required property string index
        objectName: [modelData, row, column, model, hasModelChildren, index].join(" ")
    }
}
