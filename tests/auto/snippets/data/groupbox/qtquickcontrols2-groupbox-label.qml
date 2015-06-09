import QtQuick 2.0
import QtQuick.Controls 2.0

GroupBox {
    width: 100
    height: 100
    title: "GroupBox"
    Rectangle {
        parent: label
        anchors.fill: parent
        color: 'transparent'
        border.color: 'red'
    }
}
