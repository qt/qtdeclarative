// Copyright (C) 2020 The Qt Company Ltd.

import QtQuick

Item {
    enum DummyEnum { DummyValue1, DummyValue2, DummyValue3 = 33 }
    property int value
    property Dummy child
    property Dummy2 child2 : Dummy2 {}
    property int dummyEnum

    component Group: QtObject {
        property Item item;
    }

    property Group group

    signal triggered()
    signal signalWithArg(int one, bool two)
    property real onValue
    property real offValue

    function someFunction(a: int, b: bool, c: Dummy, d: real, e: int) : int { return 42 }
    property string strProp
    function concat(a: string, b: string) : string { return a + b }
}
