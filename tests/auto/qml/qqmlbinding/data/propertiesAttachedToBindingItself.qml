import QtQml.Models
import QtQuick

Instantiator {
    id: inst
    model: 1
    property int check: 0

    delegate: Binding {
        ListView.delayRemove: true
        Component.onCompleted: inst.check += 1
    }

    Component.onCompleted: {
        if (inst.objectAt(0))
            inst.check += 2
    }
}
