pragma Strict
import QtQml

QtObject {
    objectName: a.objectName
    property QtObject a: QtObject {}
    function t() { a.objectName = "a" }
}
