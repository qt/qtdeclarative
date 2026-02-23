// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml
QtObject {
    property int value: 3
    function *gen() {
        yield value
        yield value * 2
        yield value * 3
    }
    function runGen() {
        let it = gen()
        let sum = 0
        for (let r = it.next(); !r.done; r = it.next())
            sum += r.value
        return sum
    }
}
