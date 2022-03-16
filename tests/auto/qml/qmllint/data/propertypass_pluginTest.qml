import QtQuick

Item {
    Image {
        x: 5
        Behavior on x { NumberAnimation { duration: 1000 } }
    }
}
