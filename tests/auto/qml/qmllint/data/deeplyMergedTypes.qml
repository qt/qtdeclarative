// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Each unresolved call deepens the type merge tree by one, and the branch at the end makes the
// propagator merge the whole accumulation. Without the memoization in QQmlJSTypeResolver::merge()
// the cost doubles with every line: 16 of them take 143 MB and 0.2 s, the 23 below take 2.3 GB
// and 11 s, and 24 take 3.1 GB and 21 s. Memoized it is 26 MB and 0.03 s at any of those.

import QtQml

QtObject {
    function f(text) {
        var result = text
        result = result.nope00()
        result = result.nope01()
        result = result.nope02()
        result = result.nope03()
        result = result.nope04()
        result = result.nope05()
        result = result.nope06()
        result = result.nope07()
        result = result.nope08()
        result = result.nope09()
        result = result.nope10()
        result = result.nope11()
        result = result.nope12()
        result = result.nope13()
        result = result.nope14()
        result = result.nope15()
        result = result.nope16()
        result = result.nope17()
        result = result.nope18()
        result = result.nope19()
        result = result.nope20()
        result = result.nope21()
        result = result.nope22()
        if (result.nah)
            return result
        return ""
    }
}
