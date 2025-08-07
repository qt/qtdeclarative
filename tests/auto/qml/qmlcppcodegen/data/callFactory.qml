pragma Strict
import QtQml
import TestTypes

QtObject {
    Component.onCompleted: {
        // Call the factory function and then forget the object
        const orphanedObj = CppBridge.singleObj(0)
    }
}
