import QtQuick

Window {
    width: 600
    height: 600
    visible: true
    property int delegateCreationCounter: 0

    ListView {
        id: lv
        anchors.fill: parent
        model: 6000

        delegate: Rectangle {
            width: ListView.view.width
            height: ListView.view.width / 6
            color: "white"
            border.width: 1
            Component.onCompleted: ++delegateCreationCounter
        }
    }
}
