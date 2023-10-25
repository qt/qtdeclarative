import QtQuick

Item {
    id: self
    property Item a: Item { id: a }
    Component.onCompleted: self.data = [ a ]
}
