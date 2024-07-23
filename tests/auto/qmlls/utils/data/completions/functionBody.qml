import QtQuick

Item {
    function f(x) {

    }

    property int badProperty
    component IC: Item { property int helloProperty }
    function g(x: IC) {
        x.helloProperty
        let f = () => x.helloProperty;
        let xxx = 42
        let g = () => xx
    }
}
