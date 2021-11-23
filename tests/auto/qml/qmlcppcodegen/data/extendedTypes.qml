import QtQml

QtObject {
    property int a: Qt.LeftButton + 5
    property size b: Qt.size(10, 20)
    property int c: b.width + b.height
    property string d: b.toString();
    property int e: Locale.ImperialUKSystem

    Component.onCompleted: console.log(a, b, c)
}
