// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QmlBench
import "v8-bench.js" as V8Bench

// Harness for running a V8Bench suite, API-compatible with the qmlbench tool.
// Setup/warmup happen on creation; each harness-driven tick runs the suite.
Benchmark {
    id: root

    property var suiteName
    property var suite

    Component.onCompleted: {
        var suites = V8Bench.BenchmarkSuite.suites
        for (var i = 0; i < suites.length; ++i) {
            var potentialSuite = suites[i]
            if (potentialSuite.name == root.suiteName) {
                root.suite = potentialSuite
                break;
            }
        }

        for (var i = 0; i < suite.benchmarks.length; ++i) {
            var benchmark = suite.benchmarks[i]
            benchmark.Setup();
            benchmark.run();
        }
    }

    Component.onDestruction: {
        for (var i = 0; i < suite.benchmarks.length; ++i) {
            var benchmark = suite.benchmarks[i]
            benchmark.TearDown();
        }
    }

    onTChanged: {
        for (var i = 0; i < suite.benchmarks.length; ++i) {
            var benchmark = suite.benchmarks[i]
            for (var j = 0; j < count; ++j) {
                benchmark.run();
            }
        }
    }
}
