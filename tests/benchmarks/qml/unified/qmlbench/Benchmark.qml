// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

// The base of all benchmarks, API-compatible with the qmlbench tool's Benchmark
// type. Unlike the original, `t` is NOT animated here: the C++ harness drives it
// directly so the per-tick workload runs deterministically and offscreen, with
// no dependency on a render loop.
Item {
    id: root

    // How many iterations to run "right now".
    property int count: 0

    // How many iterations to copy to 'count' (used by the original static shells;
    // kept for source compatibility with the gathered documents).
    property int staticCount: 0

    // A boolean benchmark has only two meaningful outputs: good or bad.
    property bool isBooleanResult: false

    // The constant "tick" benchmarks hook into. Driven by the harness.
    property real t
}
