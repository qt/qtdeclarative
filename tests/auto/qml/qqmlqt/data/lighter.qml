import QtQuick 2.0

QtObject {
    property var test1: Qt.lighter(Qt.rgba(1, 0.8, 0.3))
    property var test2: Qt.lighter()
    property var test3: Qt.lighter(Qt.rgba(1, 0.8, 0.3), 1.8)
    property var test4: Qt.lighter("red");
    property var test5: Qt.lighter("perfectred"); // Non-existent color
    property var test6: Qt.lighter(10);
    property var test7: Qt.lighter(Qt.rgba(1, 0.8, 0.3), 1.8, 5)

    property var testColor1: Qt.rgba(1, 0.8, 0.3).lighter()
    property var testColor3: Qt.rgba(1, 0.8, 0.3).lighter(1.8)
    property var testColor4: Qt.color("red").lighter();
}
