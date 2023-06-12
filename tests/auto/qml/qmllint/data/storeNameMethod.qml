import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    Rectangle {

        Timer {
            function foo() {}
            onTriggered: foo = 1
        }
    }
}
