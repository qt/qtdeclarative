pragma ComponentBehavior: Bound
import QtQuick

Item {
    id: root

    Loader {
        sourceComponent: Item {
            Component.onCompleted: root.objectName = "loaded"
        }
    }
}
