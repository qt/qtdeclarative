import QtQuick
import QQuickListView2TestTypes

ListView {
    id: root
    width: 100
    height: 100

    property Component typedDelegate: Item {
        width: 10
        height: 10

        required property QtObject model

        required property real a

        property real immediateX: a
        property real modelX: model.a

        function writeImmediate() {
            a = 1;
        }

        function writeThroughModel() {
            model.a = 3;
        }
    }

    property Component untypedDelegate: Item {
        width: 10
        height: 10

        property real immediateX: a
        property real modelX: model.a

        function writeImmediate() {
            a = 1;
        }

        function writeThroughModel() {
            model.a = 3;
        }
    }

    property ListModel singularModel: ListModel {
        ListElement {
            a: 11
        }
        ListElement {
            a: 12
        }
    }

    property ListModel listModel: ListModel {
        ListElement {
            a: 11
            y: 12
        }
        ListElement {
            a: 15
            y: 16
        }
    }

    property var array: [
        {a: 11, y: 12}, {a: 19, y: 20}
    ]

    property QtObject object: QtObject {
        property int a: 11
        property int y: 12
    }

    property int modelIndex: Model.None
    property int delegateIndex: Delegate.None

    model: {
        switch (modelIndex) {
        case Model.Singular: return singularModel
        case Model.List: return listModel
        case Model.Array: return array
        case Model.Object: return object
        }
        return undefined;
    }

    delegate: {
        switch (delegateIndex) {
        case Delegate.Untyped: return untypedDelegate
        case Delegate.Typed: return typedDelegate
        }
        return null
    }
}
