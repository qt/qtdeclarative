import QtQuick

Item {
    width: 640
    height: 450

    property alias tableView: tableView

    TableView {
        id: tableView
        anchors.fill: parent
        height: 300
        width: 200

        property bool success: false

        delegate: Rectangle {
            required property var model
            implicitWidth: 100
            implicitHeight: 50
            property var mydata: model?.custom ?? model.display
            onMydataChanged: tableView.success = mydata === 42
        }
    }
}
