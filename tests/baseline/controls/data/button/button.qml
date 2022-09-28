import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    width: 150
    spacing: 5

    Button {
        text: "Normal"
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 20
    }

    Button {
        text: "Focused"
        Layout.preferredWidth: 106
        Layout.preferredHeight: 50
        Layout.alignment: Qt.AlignHCenter
        focus: true
    }

    Button {
        text: "Flat"
        Layout.alignment: Qt.AlignHCenter
        flat: true
    }

    Button {
        text: "Highlighted"
        Layout.alignment: Qt.AlignHCenter
        highlighted: true
    }

    Button {
        text: "Disabled"
        Layout.alignment: Qt.AlignHCenter
        enabled: false
    }

    Button {
        text: "Down"
        Layout.alignment: Qt.AlignHCenter
        Layout.bottomMargin: 20
        down: true
    }
}
