import QtQuick 2.12

Item {
    id: root

    width: 640
    height: 480

    property bool toggle: false
    property Item bound
    property string message: "defined"

    readonly property Item item: root.toggle ? root : null

    Binding { target: root; property: "bound"; value: item}

    function tog() {
        console.info(root.bound ? root.bound.message: "undefined")
        root.toggle = !root.toggle
        return 42;
    }
}
