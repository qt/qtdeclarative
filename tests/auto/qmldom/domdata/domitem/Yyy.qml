// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtQuick as QQ

Zzz {
    id: root
    width: height
    Rectangle {
        color: "green"
        anchors.fill: parent
        height: root.foo.height
        width: root.height

    }

    function lala() {}
    property Rectangle foo: Rectangle{ height: 200 }
    function longfunction(a, b, c = "c", d = "d"): string {
        return "hehe: " + c + d
    }

    // documentedFunction: is documented
    // returns 'Good'
    function documentedFunction(arg1, arg2 = "Qt"): string {
        return "Good"
    }
    QQ.Rectangle {
        color:"red"
    }

    component IC: Zzz { property SomeBase data }
    property SomeBase mySomeBase
}
