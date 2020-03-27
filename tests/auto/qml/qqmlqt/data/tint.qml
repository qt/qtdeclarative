import QtQuick 2.0

QtObject {
    property color test1: Qt.tint("red", "blue");
    property color test2: Qt.tint(Qt.rgba(1, 0, 0), Qt.rgba(0, 0, 0, 0));
    property color test3: Qt.tint("red", Qt.rgba(0, 0, 1, 0.5));
    property color test4: Qt.tint("red", Qt.rgba(0, 0, 1, 0.5), 10);
    property color test5: Qt.tint("red")

    property color testColor1: Qt.color("red").tint("blue");
    property color testColor2: Qt.rgba(1, 0, 0).tint(Qt.rgba(0, 0, 0, 0));
    property color testColor3: Qt.color("red").tint(Qt.rgba(0, 0, 1, 0.5));
    property color testColor5: Qt.color("red").tint()
}
