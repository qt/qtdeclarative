import QtQuick

Item {
    id: root

    property alias backButton: backButton

    Item {
        id: backButton
        signal clicked()
    }
}