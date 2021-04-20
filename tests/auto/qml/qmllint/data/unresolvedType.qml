import QtQuick 2.0

Item {
    UnresolvedType {
        unresolvedProperty: 5
        Item {}
        Item {}
    }
    Repeater {
        model: 10
        delegate: UnresolvedType {}
    }
    property UnresolvedType foo: Item {}
}
