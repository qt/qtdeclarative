import QtQuick 2.0

Item {
    property int testa1: i1.p1
    property int testa2: -testa1 - i1.p1

    property int testb1: i1.p1 & 2
    property int testb2: i1.p2 & 2
    property int testb3: 2 & i1.p1
    property int testb4: 2 & i1.p2
    property int testb5: i1.p1 & i1.p3
    property int testb6: i1.p2 & i1.p3
    property int testb7: i1.p3 & i1.p1
    property int testb8: i1.p3 & i1.p2

    property int testc1: i1.p1 | 2
    property int testc2: i1.p2 | 2
    property int testc3: 2 | i1.p1
    property int testc4: 2 | i1.p2
    property int testc5: i1.p1 | i1.p3
    property int testc6: i1.p2 | i1.p3
    property int testc7: i1.p3 | i1.p1
    property int testc8: i1.p3 | i1.p2

    property int testd1: i1.p1 ^ 7
    property int testd2: 7 ^ i1.p1
    property int testd3: i1.p1 ^ i1.p4
    property int testd4: i1.p4 ^ i1.p1

    property int teste1: i1.p4 << 2
    property int teste2: i1.p5 << 2
    property int teste3: 2 << i1.p4
    property int teste4: i1.p4 << i1.p3
    property int teste5: i1.p5 << i1.p3
    property int teste6: i1.p3 << i1.p4

    property int testf1: i1.p4 >> 2
    property int testf2: i1.p5 >> 2
    property int testf3: 2 >> i1.p4
    property int testf4: i1.p4 >> i1.p3
    property int testf5: i1.p5 >> i1.p3
    property int testf6: i1.p3 >> i1.p4

    property int testg1: i1.p4 >>> 2
    property int testg2: i1.p5 >>> 2
    property int testg3: 2 >>> i1.p4
    property int testg4: i1.p4 >>> i1.p3
    property int testg5: i1.p5 >>> i1.p3
    property int testg6: i1.p3 >>> i1.p4

    QtObject {
        id: i1
        property int p1: 333
        property int p2: -666
        property int p3: 2
        property int p4: 7
        property int p5: -7
    }
 }
