// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml

QtObject {
    id: self
    property int a: 3
    property var result
    Component.onCompleted: {
        var sum = 0
        let f = function() {
            return self.notthere ?? self.a
        }

        // Not enough times for the jit to kick in (should run on the interpreter)
        for (let i = 0; i < 1; i++) {
            sum = sum + f()
        }
        result = sum
    }
}
