import QtQuick

Item {
    id: root

    property alias backButton: header.backButton
    property var header

    header: NavBar {
        id: header
    }

    signal newTaskCreated()
    signal taskUpdated()
}