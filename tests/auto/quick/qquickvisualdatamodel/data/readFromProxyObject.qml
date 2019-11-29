import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    id: window
    width: 200
    height: 200
    color: "red"
    visible: true
    Repeater {
        model: Qt.application.screens
        Text {
            required property string name
            required property int virtualX
            required property int virtualY

            text: name + virtualX + ", " + virtualY
            Component.onCompleted: window.name = name
        }
    }

    property string name: "wrong"
}
