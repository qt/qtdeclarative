import QtQuick

Item {
    function f(x) {

    }

    property int badProperty
    component IC: Item { property int helloProperty }
    function g(x: IC) {
        x.helloProperty
    }
}
