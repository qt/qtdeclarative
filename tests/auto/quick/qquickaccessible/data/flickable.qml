import QtQuick

Flickable {
    objectName: "flickable"
    width: 200
    height: 200
    contentWidth: 400
    contentHeight: 1000

    Accessible.role: Accessible.Pane
    Accessible.name: "Flickable"
}
