import QtQml

QtObject {
    property real good: 10.1
    property real bad: "f" / 10

    property bool a: isNaN(good)
    property bool b: isNaN(bad)
}
