import QtQuick
import QtQuick.Shapes
import QtQuick.Controls

Item {
    id: root
    width: 500
    height: 500
    Flickable {
        anchors.centerIn: parent
        width: 100
        height: 100
        clip: true
        contentWidth: content.width
        contentHeight: content.height
        Rectangle {
            id: content
            width: 320
            height: width
            color: "#41cd52"
            radius: width/2
        }
    }
}
