import QtQml
import bindable 1.0

WithBinding {
    id: root
    property int trigger: 1
    property alias aliasProp: root.prop
}
