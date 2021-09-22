pragma Strict
import QtQml

QtObject {
    id: self
    property list<QtObject> items
    property int numItems: items.length

    Component.onCompleted: {
        items.length = 3
        for (var i = 0; i < 3; ++i)
            items[i] = self
    }
}
