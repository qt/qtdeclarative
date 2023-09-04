import QtQml

QtObject {
    id: root
    signal _foo

    property int handled: 0

    property Connections c: Connections {
        target: root
        function on_Foo() { root.handled += 1 }
        function on_foo() { root.handled += 2 }
    }
}

