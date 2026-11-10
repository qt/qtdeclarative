import QtQuick

Item {
    id: root
    property int foo: 1
    function getFoo() { return foo }

    property QtObject target: withoutFoo

    readonly property QtObject withFoo: QtObject {
        property int foo: 2
        function getFoo() { return foo }
        property int result
        property int callResult
    }

    readonly property QtObject withoutFoo: QtObject {
        property int result
        property int callResult
    }

    states: State {
        name: "changed"
        PropertyChanges {
            target: root.target
            result: foo
            callResult: getFoo()
        }
    }
}
