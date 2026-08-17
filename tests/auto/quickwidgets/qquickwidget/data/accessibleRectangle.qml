import QtQuick

Item {
    width: 200
    height: 200

    Rectangle {
        objectName: "rect"
        anchors.fill: parent
        color: "red"

        Accessible.role: Accessible.Graphic
        Accessible.name: "rect"
    }
}
