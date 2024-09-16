import Qt.test
import QtQml

QtObject {
    id: root

    property int a: 0
    property int b: 0

    signal someSignal

    function destroyObj() {
        obj.destroy()
    }

    function test() {
        ++a
    }

    component DestructionReceiver: QtObject {
        // Has its own context and therefore can receive Component.onDestruction
    }

    property QtObject obj: QtObject {
        property QtObject inner: DestructionReceiver {
            Component.onDestruction: {
                // The outer obj is already queued for deletion.
                // We don't want to see this signal delivered.
                root.someSignal();
                ++root.b
            }
        }
    }

    Component.onCompleted: someSignal.connect(obj, test)
}
