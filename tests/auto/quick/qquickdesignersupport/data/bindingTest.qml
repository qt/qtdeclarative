import QtQuick 2.9
import QtQuick.Window 2.3

Rectangle {
    id: rect
    width: 640
    height: 480
    color: "red"
    objectName: "root"

    Text {
        objectName: "text"
        text: qsTr("Some Text") + rect.width
        font.bold: true
        font.italic: false
    }
    Item {
        objectName: "item"
    }
}
