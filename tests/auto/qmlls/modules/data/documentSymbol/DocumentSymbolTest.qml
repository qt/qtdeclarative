import QtQuick

Item {
    // late binding
    property var a
    a: [1, 0, 3, 1]
    property var b: 1;
    property var c: 12345678901234567890123456;

    // binding children
    property list<Rectangle> d: [
        Rectangle { color: "red" },
        Rectangle { color: "blue"}
    ]
    property var e: Item{}

    //Method&Signals
    function f(){}
    signal g(a: int)
    function h(msg: string) : string {}

    // child obj & hierarchy
    Component {
        component InlCmp: Item {}
        Button {
            id: button

            enum ButtonShape { Round }
        }
    }
}
