pragma ComponentBehavior: Bound
import QtQuick

ListView {
    model: 3
    delegate: Item {}
    highlight: Item {
        objectName: "highlight"
    }
}
