import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ListView {
    model: ListModel {
        ListElement {
            text: qsTr("Option 1")
            down: true
            focus: false
            highlighted: false
            enabled: true
            checked: false
        }
        ListElement {
            text: qsTr("Option 2")
            down: false
            focus: true
            highlighted: false
            enabled: true
            checked: false
        }
        ListElement {
            text: qsTr("Option 3")
            down: false
            focus: false
            highlighted: true
            enabled: true
            checked: false
        }
        ListElement {
            text: qsTr("Option 4")
            down: false
            focus: false
            highlighted: false
            enabled: false
            checked: false
        }
        ListElement {
            text: qsTr("Option 5")
            down: false
            focus: false
            highlighted: false
            enabled: true
            checked: true
        }
        ListElement {
            text: qsTr("Option 6")
            down: false
            focus: false
            highlighted: false
            enabled: true
            checked: false
        }
    }
    delegate: RadioDelegate {
        text: model.text
        down: model.down
        focus: model.focus
        highlighted: model.highlighted
        enabled: model.enabled
        checked: model.checked
    }
}
