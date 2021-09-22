import QtQml

QtObject {
    id: self
    property list<QtObject> items
    Component.onCompleted: {
        items.push(self)
        items.push(null)
        items.push(self)
        items[0] = null
    }
}
