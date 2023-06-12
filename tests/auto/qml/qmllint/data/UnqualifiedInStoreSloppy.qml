import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    Rectangle {
        property real divisor: 0

        Timer {
            onTriggered: divisor = 1
        }
    }
}
