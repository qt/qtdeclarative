pragma Strict
import QtQml

QtObject {
    property QtObject bar: QtObject {
        id: inner
        property string a: objectName
        objectName: "foo"
    }

    objectName: inner.a
}
