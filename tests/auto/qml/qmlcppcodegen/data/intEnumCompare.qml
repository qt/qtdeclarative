pragma Strict
import QtQml

QtObject {
    property bool a: Component.Asynchronous == 1
    property bool b: Component.Asynchronous != 1
    property bool c: Qt.BottomEdge > 4
    property bool d: Qt.BottomEdge < 2
}
