import QtQml 2.0
import Test 1.0

TestObject {
    property QtObject connection: Connections {
        function onSignalWithGlobalName() { ran = true }
    }
}
