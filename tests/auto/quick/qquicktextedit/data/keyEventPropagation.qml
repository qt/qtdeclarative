import QtQuick

FocusScope {
    Component.onCompleted: edit.forceActiveFocus()

    anchors.fill: parent

    Keys.onPressed: function(event) { keyDown(event.key) }
    Keys.onReleased: function(event) { keyUp(event.key) }
    signal keyDown(int key)
    signal keyUp(int key)

    TextEdit {
        id: edit
        anchors.centerIn: parent
        width: 50
        height: 50
        Keys.onPressed: function(event) {
            event.accepted = event.key == Qt.Key_A || event.key == Qt.Key_Right
        }
        Keys.onReleased: function(event) {
            event.accepted = event.key == Qt.Key_A
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: -5
            color: "transparent"
            border.width: 1
        }
    }
}
