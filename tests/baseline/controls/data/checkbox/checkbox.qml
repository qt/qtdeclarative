import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 5

    CheckBox {
        checked: true
        text: qsTr("First")
    }

    CheckBox {
        checked: false
        text: qsTr("second")
    }

    CheckBox {
        checked: true
        text: qsTr("Third")
        enabled: false
    }

    ButtonGroup {
        id: childGroup
        checkState: parentBox.checkState
        exclusive: false
    }

    CheckBox {
        id: parentBox
        checked: true
        text: qsTr("Parent")
        tristate: true
        checkState: childGroup.checkState
    }
    CheckBox {
        checked: true
        text: qsTr("1st")
        ButtonGroup.group: childGroup
        leftPadding: indicator.width

    }
    CheckBox {
        checked: true
        text: qsTr("2nd")
        ButtonGroup.group: childGroup
        leftPadding: indicator.width

    }
}
