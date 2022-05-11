import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    width: 500
    height: 500
    spacing: 4

    RadioButton {
        checked: true
        text: qsTr("1st")
    }

    RadioButton {
        text: qsTr("2nd")
    }

    RadioButton {
        text: qsTr("3rd")
        enabled: false
    }

    RadioButton {
        text: qsTr("4th")
        focus: true
    }
}
