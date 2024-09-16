pragma Strict
import QtQml

QtObject {
    id: first
    property int value

    objectName: a.objectName
    property QtObject a: QtObject {}
    function t() { a.objectName = "a" }

    Component.onCompleted: {
        for (let i = 0; i < 10; ++i) {
            first.value = i;
        }
    }
}
