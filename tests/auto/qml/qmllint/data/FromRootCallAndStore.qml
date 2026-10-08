import QtQuick

Item {
    id: root
    property int value
    function method() { return 42 }

    Item {
        property int call: method()
        Component.onCompleted: value = 1
    }
}
