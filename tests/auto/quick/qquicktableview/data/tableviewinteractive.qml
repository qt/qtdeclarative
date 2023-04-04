import QtQuick
import QtQuick.Controls

Item {
    id: rootItem 

    width: 200
    height: 200
    visible: true

    property int eventCount: 0
    property alias tableView: tableView

    MouseArea {
        anchors.fill: parent
        onPressed: function(mouse) {
            ++eventCount
        }
    }

    TableView {
        id: tableView
        objectName: "tableView"
        anchors.fill: parent
        model: 1
        delegate: Rectangle {
            color: "red"
            implicitWidth: 200
            implicitHeight: 200
        }
    }
}
