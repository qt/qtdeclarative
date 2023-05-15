import QtQml

DelegateModel {
    id: root

    // useful object as model, string as modelData
    property ListModel singularModel: ListModel {
        ListElement {
            a: "a"
        }
        ListElement {
            a: "b"
        }
    }

    // same, useful, object as model and modelData
    property ListModel listModel: ListModel {
        ListElement {
            a: "a"
            b: "a"
        }
        ListElement {
            a: "b"
            b: "b"
        }
    }

    // useful but different objects as modelData and model
    // This is how the array accessor works. We can live with it.
    property var array: [
        {a: "a", b: "a"}, {a: "b", b: "a"}
    ]

    // string as modelData
    // object with anonymous string property as model.
    property var stringList: ["a", "b"]

    // useful but different objects as modelData and model
    // This is how the object accessor works. We can live with it.
    property QtObject object: QtObject {
        property string a: "a"
        property string b: "a"
    }

    // number as modelData
    // object with anonymous number property as model
    property int n: -1

    model: {
        switch (n) {
        case 0: return singularModel
        case 1: return listModel
        case 2: return array
        case 3: return stringList
        case 4: return object
        case 5: return n
        }
        return undefined;
    }

    delegate: QtObject {
        required property var modelData
        required property var model

        property var modelA: model.a
        property var modelDataA: modelData.a
        property var modelSelf: model
        property var modelDataSelf: modelData
        property var modelModelData: model.modelData
        property var modelAnonymous: model[""]
    }
}
