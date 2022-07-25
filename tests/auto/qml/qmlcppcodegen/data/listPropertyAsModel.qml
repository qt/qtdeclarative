pragma Strict
import QtQuick

Item {
    Item {
        id: child
        Item {}
        Item {}
        Item {}
    }
    Repeater {
        id: self
        Item {}
        Component.onCompleted: self.model = child.children
    }
}
