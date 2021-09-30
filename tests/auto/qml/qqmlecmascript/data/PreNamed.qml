import QtQml

QtObject {
    property QtObject other: QtObject {
        objectName: "original"
    }
    objectName: other.objectName
    function updateOriginal() {
        other.objectName = "updated"
    }
}
