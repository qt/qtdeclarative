import QtQml.Models
import QtQuick

Item {
    id: root
    property int count: 0
    Component {
        id: delegate
        Item {
            Component.onCompleted: root.count++
        }
    }

    Repeater { model: 10; delegate: delegate }
}
