import QtQuick
import QtQuick.Effects

MultiEffect {
    id: effect

    Component {
        id: itemComponent
        Item {}
    }

    Component.onCompleted: {
        // Assign a transient item that's dropped on the next GC round
        effect.source = itemComponent.createObject()
        effect.maskSource = itemComponent.createObject()
    }
}
