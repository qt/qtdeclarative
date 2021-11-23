import QtQml

QtObject {
    objectName: "foo"
    Component.onCompleted: objectName = objectName + "bar"
}
