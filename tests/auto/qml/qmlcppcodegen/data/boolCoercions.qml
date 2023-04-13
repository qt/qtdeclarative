pragma Strict
import QtQml

QtObject {
    property rect a
    property bool t1: a

    property int c: 1
    property bool t2: c

    property url e: "qrc:/ab/c.txt"
    property bool t3: e

    property string f: "a"
    property bool t4: f

    property var j: 1
    property bool t5: j

    id: k
    property bool t6: k

    property date l
    property bool t7: l

    property url m
    property bool t8: m



    property int b: 0
    property bool f1: b

    property QtObject d: null
    property bool f2: d

    property string g
    property bool f3: g

    property var h: undefined
    property bool f4: h

    property var i: null
    property bool f5: i
}
