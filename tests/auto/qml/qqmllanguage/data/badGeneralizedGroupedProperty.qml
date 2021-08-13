import QtQml

QtObject {
    id: root
    objectName: "foo"

    property QtObject child: QtObject {
        id: child
        objectName: "barrrrr"
        root.objectName: objectName + " ..."
    }
}
