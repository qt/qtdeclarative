import QtQuick

Item {
    id: root
    property bool toggle: false
    property int i: 42
    property string text: "original"
    Loader {
        active: root.toggle
        sourceComponent: Binding {
            target: root
            property: "text"
            value: "changed"
            restoreMode: Binding.RestoreBindingOrValue
        }
    }
    Loader {
        active: root.toggle
        sourceComponent: Binding {
            target: root
            property: "i"
            value: 100
            restoreMode: Binding.RestoreNone
        }
    }
}
