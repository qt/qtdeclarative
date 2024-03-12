pragma Strict
pragma ValueTypeBehavior: Addressable

import QtQuick

GOL_Object {
    id: root

    property int p1: 5 ?? -1
    property string p2: "6" ?? "-1"

    property var p3: undefined ?? undefined
    property var p4: undefined ?? null
    property var p5: undefined ?? -1
    property var p6: undefined ?? "-1"

    property var p7: null ?? undefined
    property var p8: null ?? null
    property var p9: null ?? -1
    property var p10: null ?? "-1"

    property int p11: GOL_Object.V2 ?? "-1"

    property int p12: 1 ?? 2 ?? 3
    property int p13: "1" ?? "2" ?? "3"
    property var p14: undefined ?? "2" ?? undefined
    property var p15: undefined ?? undefined ?? 1

    property var p16
    property var p17

    Component.onCompleted: {
        p16 = (root.childA as GOL_Object)?.i ?? -1
        root.childA = root
        p17 = (root.childA as GOL_Object)?.i ?? -1
    }
}
