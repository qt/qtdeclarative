import QtQml
import Test

TestObject {
    id: root
    property int foo: 1
    function getFoo() { return foo }

    readonly property QtObject withFoo: QtObject {
        property int foo: 2
        function getFoo() { return foo }
    }
    readonly property QtObject withoutFoo: QtObject {}
    readonly property QtObject outer: QtObject {
        property int foo: 3
        function getFoo() { return foo }
    }

    readonly property TestObject load: TestObject { scriptString: foo }
    readonly property TestObject call: TestObject { scriptString: getFoo() }
}
