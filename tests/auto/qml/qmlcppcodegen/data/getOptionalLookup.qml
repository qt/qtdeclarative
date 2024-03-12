pragma Strict
pragma ValueTypeBehavior: Addressable

import QtQuick

GOL_Object {
    id: root

    property rect r: Qt.rect(0, 0, 20, 50)
    property point p: Qt.point(0, -10)
    property var v: Qt.point(5, 5)
    property var u: undefined

    property int to1: root?.i
    property string to2: root?.s
    property GOL_Object to3: root?.childA
    property var to4: root.childA?.i
    property var to5: (undefined as GOL_Object)?.childA
    property int to6: (root as GOL_Object)?.s.length

    property int tv1: root.r?.bottom
    property int tv2: root.p?.y

    property int te1: root?.e
    property int te2: GOL_Object?.V2
    property bool te3: root?.e === GOL_Object?.V1
    property bool te4: root?.e === GOL_Object?.V2

    property int tc1: root?.p.y
    property int tc2: root.r?.x

    property var tc4: root?.childA?.s
    property var tc5: root.childA?.s
}
