import QtQuick
import QtQuick.Layouts

Item {
    id: rootId
    property var test3
    property var test1: 1
    property var test1b
    test2: 2


    enum MyEnum {
        HELLO = 20,
        WORLD = 40,
        ABC_END = 500
    }

    Item {
        //another item
    }

test3: 3 // comment on test3
    d: 1
     b: 2
     f: 3
    a: 4
     border.color: "red"
    border.width: 2



    function onValueChanged() {
        doStuff()
    }

Item2 { }Item3 { }
}