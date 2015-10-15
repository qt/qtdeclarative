import QtQuick 2.0
import QtQuick.Layouts 1.0
import Qt.labs.controls 1.0

ToolBar {
    RowLayout {
        anchors.fill: parent
        ToolButton {
            text: qsTr("\u25C0 Qt")
            onClicked: stack.pop()
        }
        Item { Layout.fillWidth: true }
        Switch {
            checked: true
            text: qsTr("Notifications")
        }
    }
}
