import QtQuick 2.0

QtObject {
    property variant test1: Qt.alpha(Qt.rgba(1, 0.8, 0.3), 0.5)
    property variant test2: Qt.alpha()
    property variant test3: Qt.alpha(Qt.rgba(1, 0.8, 0.3), 0.7)
    property variant test4: Qt.alpha("red", 0.5);
    property variant test5: Qt.alpha("perfectred", 0.5); // Non-existent color
    property variant test6: Qt.alpha(1, 0.5);
    property variant test7: Qt.alpha(Qt.rgba(1, 0.8, 0.3), 2.8, 10)

    property variant testColor1: Qt.rgba(1, 0.8, 0.3).alpha(0.5)
    property variant testColor3: Qt.rgba(1, 0.8, 0.3).alpha(0.7)
    property variant testColor4: Qt.color("red").alpha(0.5);
}
