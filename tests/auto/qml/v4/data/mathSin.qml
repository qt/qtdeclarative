import QtQuick 2.0

Item {
    property real test1: Math.sin(i1.p1)
    property real test2: Math.sin(i1.p2)

    property real subtest3: Math.sin()
    property real subtest4: Math.sin(i1.p4)
    property bool test3: isNaN(subtest3)
    property bool test4: isNaN(subtest4)

    property real subtest5: Math.sin(i1.p5)
    property bool test5: isNaN(subtest5)
    property real test6: Math.sin(i1.p6)

    property real subtest7: Math.sin(i1.p7)
    property real subtest8: Math.sin(i1.p8)
    property bool test7: isNaN(subtest7)
    property bool test8: isNaN(subtest8)

    property real subtest9: Math.sin(i1.p9)
    property bool test9: subtest9 === 0 && (1/subtest9) === Infinity
    property real subtest10: Math.sin(i1.p10)
    property bool test10: subtest10 === 0 && (1/subtest10) === -Infinity

    property real subtest11: Math.PI / 6.66
    property real test11: Math.sin(subtest11)

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
