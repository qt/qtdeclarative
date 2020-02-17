import QtQuick 2.12
import QtQuick.Window 2.12

Window {
    id: root
    property int statusChangedCounter: 0
    property alias status: loader.status
    visible: true; width: 640; height: 480
    Loader {
        id: loader
        anchors.fill: parent
        asynchronous: true
        source: "./RedRect.qml"
        onStatusChanged: root.statusChangedCounter++
    }
}
