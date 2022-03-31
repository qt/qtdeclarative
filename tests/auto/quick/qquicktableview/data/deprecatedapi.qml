import QtQuick

Item {
    width: 400
    height: 300
    visible: true

    property alias tableView: tableView

    TableView {
        id: tableView
        anchors.fill: parent
        delegate: Rectangle {
            implicitWidth: 40
            implicitHeight: 20
            border.width: 1
        }

        function positionUsingDeprecatedEnum()
        {
            // Using Qt.Alignment for the second argument is deprecated, but supported
            positionViewAtCell(columns - 1, rows - 1, Qt.AlignBottom | Qt.AlignRight)
        }
    }
}
