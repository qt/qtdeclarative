import QtQuick

Item {
    id: root
    property int v: 0

    Transition {
        id: transition
        NumberAnimation {}
    }

    Instantiator {
        id: instantiator

        delegate: NumberAnimation {}

        onObjectAdded: (index, object) => {
            transition.animations.splice(index, 0, object)
        }

        onObjectRemoved: (index, object) => {
            // NB: This is a logic error. The indices of other entries change when we remove one
            //     from the middle, but Instantiator will still report the old indices.
            //     We still want to do this, in order ot exercise the code that nulls dangling
            //     pointers.
            transition.animations.splice(index, 1)
        }
    }

    Timer {
        interval: 2
        running: true
        repeat: true
        onTriggered: {
            instantiator.model = (++v % 2) ? 0 : 10
        }
    }
}


