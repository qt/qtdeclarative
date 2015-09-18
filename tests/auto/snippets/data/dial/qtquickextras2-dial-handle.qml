import QtQuick 2.0
import QtQuick.Controls 2.0

Dial {
    id: dial

    Rectangle {
        parent: dial.handle
        anchors.fill: parent
        color: 'transparent'
        border.color: 'red'
        antialiasing: true
    }
}
