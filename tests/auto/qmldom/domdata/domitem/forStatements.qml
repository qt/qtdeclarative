// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick

Item {
    function f() {
        let sum = 0, helloWorld = "hello"
        for (let i = 0; i < 100; i = i + 1) {
            sum = sum + 1
            for (;;)
                i = 42
        }
    }

    // note: not supported in 6.6 but should not crash in 6.6
    function ff(a, b, c) {
        if (a) {
            b = a + b;
        } else {
            a = a + myItem.i;
        }
        let sum = 0;
        for (let i = a; i < b; i = i + c) {
            while (a) {
                sum = sum + b - c + a * i;
                {
                    let i = 32 // not a definition nor usage of i
                    i = 44 // neither
                }
            }
        }
        console.log(sum);
    }
}
