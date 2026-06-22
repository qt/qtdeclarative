// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QmlBench

// Evaluation-bound benchmark with branching typed expressions (ternaries, modulo)
// on top of arithmetic. Same idea as EvalArithmetic: a fixed tree re-evaluated on
// each `t` tick, so the run phase measures binding evaluation rather than
// allocation. AOT-compiled C++ bindings should beat the bytecode of source/qmlc.
Benchmark {
    id: root
    count: 2000

    Component.onCompleted: rep.model = root.count

    Repeater {
        id: rep
        Item {
            required property int index
            property real p: root.t + index
            property real q: p > 100 ? p * 1.5 + index * 0.3 : p * 0.5 - index * 0.2
            x: q * 2.0 - p + root.t
            y: q + p * 0.25 - index * 0.01
            opacity: (index % 2 === 0) ? (q * 0.0001) : (p * 0.0001)
        }
    }
}
