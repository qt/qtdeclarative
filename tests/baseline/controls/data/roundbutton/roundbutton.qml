import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 5

    RoundButton {
    }

    RoundButton {
        text: qsTr("2")
        enabled: false
    }

    RoundButton {
        text: qsTr("3")
        down: true
    }

    RoundButton {
        text: qsTr("4")
        checked: true
    }

    RoundButton {
        text: qsTr("5")
        checkable: true
    }

    RoundButton {
        text: qsTr("6")
        focus: true
    }

    RoundButton {
        text: qsTr("7")
        highlighted: true
    }

    RoundButton {
        text: qsTr("8")
        flat: true
    }

    RoundButton {
        text: qsTr("9")
        LayoutMirroring.enabled: true
    }
}
