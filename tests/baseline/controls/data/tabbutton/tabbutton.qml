import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    TabBar {

        TabButton {
            text: qsTr("Button 1")
        }

        TabButton {
            text: qsTr("Button 2")
            enabled: false
        }

        TabButton {
            text: qsTr("Button 3")
            focus: true
        }

        TabButton {
            text: qsTr("Button 5")
            down: true
        }

        TabButton {
            text: qsTr("Button 6")
            checked: true
        }

        TabButton {
            text: qsTr("Button 7")
            LayoutMirroring.enabled: true
        }
    }
}
