import QtQml

QtObject {
    property QtObject a: QtObject {}
    property QtObject b: a as SomethingElse
}
