pragma ValueTypeBehavior: Assertable
import QtQml as Q
import StaticTest as S

Q.QtObject {
    property var a
    property rect b: a as Q.rect
    property bool c: a instanceof Q.rect
    property bool d: ({x: 10, y: 20}) instanceof Q.point
    property var e: ({x: 10, y: 20}) as Q.point
    property var f: "red" as S.withString
    property var g: "green" as Q.string

    property var h: new S.withString("red")
    property var i: {
        let p = new Q.point;
        p.x = 10
        p.y = 20
        return p
    }

    property var j: 4.0 as Q.int
    property var k: (4.5 / 1.5) as Q.int
    property var l: 5 as Q.double
    property var m: "something" as Q.var
    property var n: 1 as Q.bool
    property var o: Infinity as Q.int

    property var p: b as Q.size;
    property var q: this as Q.size;
    property var r: ({}) as Q.size;
    property var s: 11 as Q.size;
    property var t: Q.Component as Q.size;
    property var u: Q.Qt as Q.size;
}
