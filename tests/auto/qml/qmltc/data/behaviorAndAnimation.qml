import QtQuick
Rectangle {
    width: 100
    Behavior on width {
        NumberAnimation { duration: 1000 }
    }
}
