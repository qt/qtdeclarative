import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 2
    width: 200

    Switch {
        text: qsTr("First")
    }

    Switch {
        text: qsTr("Second")
        checked: true
    }

    Switch {
        text: qsTr("Third")
        enabled: false
    }

    Switch {
        text: qsTr("Fourth")
        LayoutMirroring.enabled: true
    }

    Switch {
        text: qsTr("Sixth")
        focus: true
    }

    Switch {
        text: qsTr("Seventh")
        down: true
    }
}
