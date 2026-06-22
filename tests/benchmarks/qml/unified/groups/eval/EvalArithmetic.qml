// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QmlBench

// Evaluation-bound benchmark. Unlike the qmlbench creation benchmarks (which are
// dominated by QQuickItem allocation), this builds a fixed tree ONCE and then, on
// every `t` tick, re-evaluates a large number of typed-arithmetic bindings WITHOUT
// allocating anything. The run phase therefore measures binding evaluation, which
// is what ahead-of-time compilation actually optimizes: in the AOT bundle these
// bindings become compiled C++ functions, whereas source/qmlc run the same
// interpreted/JIT bytecode. Expected ordering for run: aot < source ≈ qmlc.
Benchmark {
    id: root
    count: 2000

    // Build the tree once; the tick does not change the model, so nothing is
    // re-created — only the bindings below re-run.
    Component.onCompleted: rep.model = root.count

    Repeater {
        id: rep
        Item {
            required property int index
            // Pure typed real arithmetic dependent on `t` and `index`: the AOT
            // compiler resolves these fully and emits C++.
            property real a: root.t * 1.3 + index * 0.7
            property real b: a * 2.1 - root.t * 0.5
            x: b + index * 0.25 - root.t * 1.1
            y: a * 0.5 + b * 0.25 + index * 0.001
            width: x * 0.5 - y * 0.5 + root.t
            height: width * 0.3 + a - b
        }
    }
}
