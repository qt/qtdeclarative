import QtQuick 2.0

Item {
    id: root
    property MyComponent myProperty
    Binding {
        target: root
        property: "myProperty"
        value: myObject
    }
    MyComponent { id: myObject }
}
