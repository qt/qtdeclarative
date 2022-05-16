import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    width: 200
    height: 100

    Button {
        id: button
        text: qsTr("Tooltip")
        ToolTip {
            parent: button
            visible: true
            text: qsTr("Click the button")
        }
    }
}


