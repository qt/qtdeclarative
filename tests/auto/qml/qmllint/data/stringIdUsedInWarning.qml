import QtQml

QtObject {
    id: "stringy"
    property int i
    property QtObject o: QtObject {
        Component.onCompleted: console.log(i)
    }
}
