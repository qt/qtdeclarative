// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15

Item {
    component C: Item {}

    property int a
    property bool b
    property C c
    property var d
    property list<int> e

    component D: Item { id: icid }
    C {id: firstC }D{id: firstD }
    C { id: secondC }   D{ id: secondD}
    C {
        C{}
        C{
            C {}
            C {}
            C {}
        }
        C{}
    }

    component IC: Item { property C myC }

    a: 43
    d: 123 + 7

    function f(a: int, b: Item, c: C) : C {
        return c;
    }
    function lala() {}

    Rectangle {
        color: "green"
        anchors.fill: parent
        width: root.height
        height: root.foo.height
    }
    property Rectangle foo: Rectangle{ height: 200 }

    c: C{}
    property C ccc: c
    property C cccc
    cccc: c

    function fff(a: int, b: Item, c: C) : C {
        let x = c.children;
        return x;
    }
}
