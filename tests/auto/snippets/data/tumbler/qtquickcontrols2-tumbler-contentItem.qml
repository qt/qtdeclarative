import QtQuick 2.0
import QtQuick.Controls 2.0

Tumbler {
    model: 5

    Rectangle {
        anchors.fill: parent.contentItem
        color: 'transparent'
        border.color: 'red'
    }
}
