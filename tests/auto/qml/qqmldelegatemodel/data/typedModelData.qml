import QtQml

DelegateModel {
    id: root

    // useful object as model, int as modelData
    property ListModel singularModel: ListModel {
        ListElement {
            x: 11
        }
        ListElement {
            x: 12
        }
    }

    // same, useful, object as model and modelData
    property ListModel listModel: ListModel {
        ListElement {
            x: 13
            y: 14
        }
        ListElement {
            x: 15
            y: 16
        }
    }

    // useful but different objects as modelData and model
    // This is how the array accessor works. We can live with it.
    property var array: [
        {x: 17, y: 18}, {x: 19, y: 20}
    ]

    // useful but different objects as modelData and model
    // This is how the object accessor works. We can live with it.
    property QtObject object: QtObject {
        property int x: 21
        property int y: 22
    }

    property int n: -1

    model: {
        switch (n) {
        case 0: return singularModel
        case 1: return listModel
        case 2: return array
        case 3: return object
        }
        return undefined;
    }

    delegate: QtObject {
        required property point modelData
        required property QtObject model

        property real modelX: model.x
        property real modelDataX: modelData.x
        property point modelSelf: model
        property point modelDataSelf: modelData
        property point modelModelData: model.modelData
        property point modelAnonymous: model[""]
    }
}
