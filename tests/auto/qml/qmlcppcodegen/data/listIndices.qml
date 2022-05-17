pragma Strict
import QtQml

QtObject {
    id: self
    property list<QtObject> items
    property int numItems: items.length
    property QtObject fractional: items[2.25]
    property QtObject negativeZero: items[-1 * 0]
    property QtObject infinity: items[1 / 0]
    property QtObject nan: items[1 - "a"]

    Component.onCompleted: {
        items.length = 3
        for (var i = 0; i < 3; ++i)
            items[i] = self

        items[2.25] = null
        items[1 / 0] = self
        items[1 - "a"] = self
    }
}
