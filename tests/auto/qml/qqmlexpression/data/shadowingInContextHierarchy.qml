import QtQml
import Test

QtObject {
    readonly property Component withName: QtObject {
        property int foo
        function getFoo() { return foo }
    }
    readonly property Component withoutName: QtObject {}

    readonly property TestObject load: TestObject { scriptString: foo }
    readonly property TestObject call: TestObject { scriptString: getFoo() }
}
