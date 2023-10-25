import QtQml

QtObject {
    objectName: "parent"
    property list<QtObject> child
    property Component c: QtObject { objectName: "child" }

    function doCreate() {
        child.push(c.createObject(null));
    }

    Component.onCompleted: {
        // Extra function call so that the created object cannot be on the stack
        doCreate();
        gc();
    }
}
