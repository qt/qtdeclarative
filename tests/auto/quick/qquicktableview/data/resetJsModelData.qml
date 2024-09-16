import QtQuick

Item {
    width: 100
    height: 300

    property alias tableView: tableView

    TableView {
        id: tableView
        anchors.fill: parent
        property int modelUpdated: 0
        onModelChanged: { ++modelUpdated }
        delegate: Item {
            implicitHeight: 10
            implicitWidth: 10
        }
    }
}
