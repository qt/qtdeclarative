import QtQuick

Item {
    id: base
    Item {
        id: inner
        Item {
            id: child
            anchors.fill: {
                base.notified = parent;
                return parent;
            }
        }
    }

    property var notified
    Component.onCompleted: inner.destroy()
}
