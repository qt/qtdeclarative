import QtQuick

Item {
    id: root
    property bool toggle: true
    property bool forceEnable: false

    Item {
        id: item1
        property int i
    }

    Item {
        id: item2
    }

    Binding {
        target: root.toggle ? item1 : item2 
        when:  root.forceEnable || (root.toggle ? item1 : item2).hasOwnProperty("i")
        property: "i"
        value: 42
    }
}
