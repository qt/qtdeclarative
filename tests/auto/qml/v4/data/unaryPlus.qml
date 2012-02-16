import QtQuick 2.0

Item {
    property real test1: +i1.p2
    property int test2: +i1.p2
    property real test3: +i1.p1
    property int test4: +i1.p1
    property real test5: +i1.p3
    property int test6: +i1.p3
    property real test7: +i1.p4
    property int test8: +i1.p4
    property real test9: +i1.p5
    property int test10: +i1.p5

    QtObject {
        id: i1
        property real p1: -3.7
        property int p2: 18
        property real p3: -3.3
        property int p4: -7
        property real p5: 4.4
    }
}

