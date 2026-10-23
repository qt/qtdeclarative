pragma Strict
import QtQml
import TestTypes

QtObject {
    function returnOwnedObject() : CppObj {
        return CppBridge.ownedObject;
    }

    Component.onCompleted: {
        // Call the factory function and then forget the object
        const orphanedObj = CppBridge.singleObj(0)

        // Retrieve the owned object and forget that one. It should not be deleted
        objectName = returnOwnedObject().objectName;
    }
}
