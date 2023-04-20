import QtQuick
import QtQuick.Controls

Item {
    id: root
    objectName: "root"
    visible: true

    anchors.fill: parent
    property string text: ""

    ScrollView {
        anchors.fill: parent
        Rectangle {
            x: 100
            width: 100
            height: 30
            color: "lightgray"

            Text {
                objectName: "textOutsideViewport"
                text: root.text
                width: 50
                clip: true
            }
        }
    }
}
