// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
}
