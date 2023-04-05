// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick

Item {
    property int a: 42
    property int b

    b: a
    function f() {
        let sum = 0, helloWorld = "hello"
        for (let i = 0; i < 100; i = i + 1) {
            sum = sum + 1
            for (;;)
                i = 42
        }
    }

    function conditional() {
        let i = 5
        if (i)
            i = 42

        if (i == 55)
            i = 32
        else
            i = i - 1

        if (i == 42) {
            i = 111
        }

        if (i == 746) {
            i = 123
        } else {
            i = 456
        }

    }

    function testForNull() {
        for (;;)
        {}
        for (;;)
            x
        {} {} {} {}
    }
    signal mySignal()
    signal mySignal2()

    // onMySignal: function () { let ready = 0, set = 1, go = 2; } // function expressions not implemented yet!
    onMySignal2: f

}
