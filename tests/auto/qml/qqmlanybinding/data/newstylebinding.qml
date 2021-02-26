import QtQml
import bindable 1.0

WithBinding {
    id: root
    property int trigger: 1
    prop: trigger
    property alias a2: obj1.a1

    property QtObject obj: QtObject {
        id: obj1
        property alias a1: root.prop
    }
}
