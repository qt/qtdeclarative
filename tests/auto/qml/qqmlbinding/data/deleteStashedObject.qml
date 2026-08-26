pragma ComponentBehavior: Bound
import QtQml

QtObject {
    id: root

    property Component c: Component {
        QtObject {
            Component.onDestruction: {
                console.log("dead")
                timer.start()
            }
        }
    }

    property QtObject stashed: c.createObject()
    property QtObject replacement: QtObject {}

    onStashedChanged: {
        if (stashed) {
            page = stashed
            console.log("alive")
            binding.when = true
        }
    }

    property QtObject page

    property Binding binding: Binding {
        id: binding
        target: root
        property: "page"
        when: false
        value: root.replacement
    }

    property Timer timer: Timer {
        id: timer
        interval: 10
        onTriggered: {
            console.log("before")
            binding.when = false
            console.log("after")
        }
    }

    Component.onCompleted: {
        console.log("destroy")
        stashed.destroy();
    }
}
