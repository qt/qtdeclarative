import QtQuick 2.0

Item {
    property real test1: Math.abs(i1.p1)
    property real test2: Math.abs(i1.p2)

    property int test3: Math.abs(i1.p3)
    property int test4: Math.abs(i1.p4)

    property real subtest5: Math.abs()
    property real subtest6: Math.abs(i1.p6)
    property bool test5: isNaN(subtest5)
    property bool test6: isNaN(subtest6)

    property real subtest7: Math.abs(i1.p7)
    property bool test7: isNaN(subtest7)
    property int test8: Math.abs(i1.p8)

    property real subtest9: Math.abs(i1.p9)
    property real subtest10: Math.abs(i1.p10)
    property bool test9: subtest9 === Number.POSITIVE_INFINITY
    property bool test10: subtest10 === Number.POSITIVE_INFINITY

    property int test11: Math.abs(i1.p11)
    property real subtest12: Math.abs(i1.p12)
    property bool test12: subtest12 === 0 && (1/subtest12) === Infinity

    QtObject {
        id: i1
        property real p1: -3.7
        property real p2: 4.5
        property int p3: 18
        property int p4: -72
        property real p6: Number.NaN
        property string p7: "hello world"
        property string p8: "82"
        property real p9: Number.NEGATIVE_INFINITY
        property real p10: Number.POSITIVE_INFINITY
        property real p11: 0
        property real p12: -0
    }
 }
