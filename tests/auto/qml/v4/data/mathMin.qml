import QtQuick 2.0

Item {
    property real test1: Math.min(i1.p1, i1.p2)
    property real test2: Math.min(i1.p2, i1.p3)

    property real subtest3: Math.min()
    property real subtest4: Math.min(i1.p4)
    property bool test3: subtest3 === Infinity
    property bool test4: isNaN(subtest4)

    property real subtest5: Math.min(i1.p5, i1.p1)
    property bool test5: isNaN(subtest5)
    property real test6: Math.min(i1.p6, i1.p3)

    property real subtest7: Math.min(i1.p7, i1.p2)
    property bool test7: subtest7 === Number.NEGATIVE_INFINITY
    property real test8: Math.min(i1.p8, i1.p2)

    property real subtest9: Math.min(i1.p10, i1.p9)
    property bool test9: subtest9 === 0 && (1/subtest9) === -Infinity

    // Reverse the inputs to Math.min
    property real subtest10: Math.min(i1.p9, i1.p10)
    property bool test10: subtest10 === 0 && (1/subtest10) === -Infinity

    property real test11: Math.min(i1.p11, i1.p1)
    property real test12: Math.min(i1.p11, i1.p2)
    property real test13: Math.min(i1.p1, i1.p2, i1.p3)

    QtObject {
        id: i1
        property real p1: -3.7
        property real p2: 4.4
        property int p3: 95
        property real p4: Number.NaN
        property string p5: "hello world"
        property string p6: "82.6"
        property real p7: Number.NEGATIVE_INFINITY
        property real p8: Number.POSITIVE_INFINITY
        property real p9: 0
        property real p10: -0
        property var p11: null
    }
 }
