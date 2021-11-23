/******************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt JavaScript to C++ compiler.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
******************************************************************************/

import QtQuick

Item {
    enum DummyEnum { DummyValue1, DummyValue2, DummyValue3 = 33 }
    property int value
    property Dummy child
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
