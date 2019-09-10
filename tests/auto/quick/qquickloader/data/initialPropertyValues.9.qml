import QtQuick 2.0

Item {
    id: root
    property int i: 0
    property string s: ""

    Loader {
        id: loader
        objectName: "loader"
        onLoaded: {
            root.i = loader.item.i;         // should be 42
            root.s = loader.item.s; // should be 11
        }
    }

    Component.onCompleted: {
        loader.setSource("RequiredPropertyValuesComponent.qml", {"i": 42, "s": "hello world"});
    }
}
