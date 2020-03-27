import QtQuick 2.0

QtObject {
    property variant test1: Qt.color("red")
    property variant test2: Qt.color("#ff00ff00")
    property variant test3: Qt.color("taint") // Taint is not a valid color
    property variant test4: Qt.color(0.5)
    property variant test5: Qt.color()
    property variant test6: Qt.color("blue", 0)
}
