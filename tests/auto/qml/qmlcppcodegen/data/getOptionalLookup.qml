pragma Strict
pragma ValueTypeBehavior: Addressable

import QtQuick

GOL_Object {
    id: root

    property rect r: Qt.rect(0, 0, 20, 50)
    property point p: Qt.point(0, -10)
    property var v: Qt.point(5, 5)
    property var u: undefined

    function f(input: bool) : var {
        if (input)
            return 0
        return Qt.point(2, 2)
    }


    property int to1: root?.i
    property string to2: root?.s
    property GOL_Object to3: root?.childA
    property var to4: root.childA?.i
    property var to5: (undefined as GOL_Object)?.childA
    property int to6: (root as GOL_Object)?.s.length

    property int tv1: root.r?.bottom
    property int tv2: root.p?.y
    property int tv3: (root.v as point)?.x
    property var tv4: (root.u as rect)?.x

    property int te1: root?.e
    property int te2: GOL_Object?.V2
    property bool te3: root?.e === GOL_Object?.V1
    property bool te4: root?.e === GOL_Object?.V2

    property int tc1: root?.p.y
    property int tc2: root.r?.x
    property int tc3: (root?.v as point)?.y
    property var tc4: root?.childA?.s
    property var tc5: root.childA?.s
    property var tc6: (root?.u as rect)?.height
    property var tc7: (f(true) as point)?.x
    property var tc8: (f(false) as point)?.x
}
