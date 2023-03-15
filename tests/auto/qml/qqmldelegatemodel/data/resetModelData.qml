import QtQuick

ListView {
    id: root
    anchors.fill: parent
    property bool success: (currentItem?.mydata ?? 0) === 42
    height: 300
    width: 200

    delegate: Rectangle {
        required property var model
        implicitWidth: 100
        implicitHeight: 50
        property var mydata: model?.foo ?? model.bar
    }
}
