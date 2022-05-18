import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ListView {
    model: ListModel {
        ListElement {
            down: true
            focus: false
            highlighted: false
            enabled: true
            checked: false
        }
        ListElement {
            down: false
            focus: true
            highlighted: false
            enabled: true
            checked: false
        }
        ListElement {
            down: false
            focus: false
            highlighted: true
            enabled: true
            checked: false
        }
        ListElement {
            down: false
            focus: false
            highlighted: false
            enabled: false
            checked: false
        }
        ListElement {
            down: false
            focus: false
            highlighted: false
            enabled: true
            checked: true
        }
        ListElement {
            down: false
            focus: false
            highlighted: false
            enabled: true
            checked: false
        }
    }
    delegate: SwitchDelegate {
        down: model.down
        focus: model.focus
        highlighted: model.highlighted
        enabled: model.enabled
        checked: model.checked
    }
}
