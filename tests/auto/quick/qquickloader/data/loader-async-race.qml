import QtQuick 2.15

Item {
    id: root
    Component.onCompleted: {
        myloader.active = false
    }
    Loader {
        id: myloader
        anchors.fill: parent
        asynchronous: true
        source: "loader-async-race-rect.qml"
    }
}
