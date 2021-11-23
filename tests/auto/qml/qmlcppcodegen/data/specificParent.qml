import QtQuick

Rectangle {
    Item {
        id: child
        property real a: parent.radius
    }

    property real a: child.a
    radius: 77
}
