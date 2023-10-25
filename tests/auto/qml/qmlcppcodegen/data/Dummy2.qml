// Copyright (C) 2020 The Qt Company Ltd.

import QtQml

QtObject {
    property int value
    property Dummy2 child
    property int dummyEnum

    signal triggered()
    signal signalWithArg(int one, bool two)
    property real onValue
    property real offValue

    function someFunction(a: int, b: bool, c: Dummy2, d: real, e: int) : int { return 42 }
    property string strProp
    function concat(a: string, b: string) : string { return a + b }
}
