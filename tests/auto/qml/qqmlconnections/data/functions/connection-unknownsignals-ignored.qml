import QtQml 2.0

QtObject {
    id: root

    property Connections c1: Connections {
        target: root;
        function onNotFooBar1() {}
        ignoreUnknownSignals: true
    }

    property Connections c2: Connections {
        objectName: "connections"
        function onNotFooBar2() {}
        ignoreUnknownSignals: true
    }
}
