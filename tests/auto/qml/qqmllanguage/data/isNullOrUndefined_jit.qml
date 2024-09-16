// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml

QtObject {
    id: self
    property int a: 3
    property int result
    Component.onCompleted: {
        var sum = 0
        let f = function() {
            return self.notthere ?? self.a
        }

        // Enough times for the jit to kick in (should run on the jit)
        for (let i = 0; i < 50; i++) {
            sum = sum + f()
        }
        result = sum
    }
}
