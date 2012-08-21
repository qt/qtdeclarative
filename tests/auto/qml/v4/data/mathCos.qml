import QtQuick 2.0

Item {
    property real test1: Math.cos(i1.p1)
    property real test2: Math.cos(i1.p2)

    property real subtest3: Math.cos()
    property real subtest4: Math.cos(i1.p4)
    property bool test3: isNaN(subtest3)
    property bool test4: isNaN(subtest4)

    property real subtest5: Math.cos(i1.p5)
    property bool test5: isNaN(subtest5)
    property real test6: Math.cos(i1.p6)

    property real subtest7: Math.cos(i1.p7)
    property real subtest8: Math.cos(i1.p8)
    property bool test7: isNaN(subtest7)
    property bool test8: isNaN(subtest8)

    property real subtest9: Math.cos(i1.p9)
    property bool test9: subtest9 === 1
    property real subtest10: Math.cos(i1.p10)
    property bool test10: subtest10 === 1

    property real subtest11: Math.PI / 6.66
    property real test11: Math.cos(subtest11)

    QtObject {
        id: i1
        property real p1: -3.7
        property real p2: 4.4
        property real p4: Number.NaN
        property string p5: "hello world"
        property string p6: "82.6"
        property real p7: Number.NEGATIVE_INFINITY
        property real p8: Number.POSITIVE_INFINITY
        property real p9: 0
        property real p10: -0
    }
 }
