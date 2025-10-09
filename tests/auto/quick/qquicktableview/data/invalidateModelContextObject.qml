import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    width: 720
    height: 480
    visible: true

    property alias tableView: tableView
    property int modelData: 15

    Page {
        anchors.fill: parent
        header: Rectangle {
            height: 40
            color: "red"
        }
        TableView {
            id: tableView
            anchors.fill: parent
            model: modelData
            contentY: Math.max(0, contentHeight - height)
            contentHeight: 40 * rows
            rowHeightProvider: () => 40
            columnWidthProvider: () => 200
            delegate : Rectangle {
                width: 40;
                height: 40;
                color: "green"
                Text {
                    anchors.fill: parent
                    text: index
                }
            }
        }
    }
}
