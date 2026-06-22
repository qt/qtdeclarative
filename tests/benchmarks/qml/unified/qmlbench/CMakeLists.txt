# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QmlBench harness module.
##
## Reproduces the QmlBench module from the qmlbench tool so the gathered
## benchmark documents can be used verbatim. Unlike the original, the `t` tick
## is driven from C++ (see Benchmark.qml), so per-tick workloads run
## deterministically and offscreen without a render loop.
##
## Built as a static QML module (NO_PLUGIN: everything is linked into each mode
## executable, so types register statically and no plugin is loaded at runtime).
## It is the shared dependency of the measured bundles, reachable at the standard
## user-module path :/qt/qml/QmlBench/.
#####################################################################

unified_add_qml_module(BenchmarkQmlBench
    URI QmlBench
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/QmlBench"
    SOURCES
        qmlbenchsingleton.h qmlbenchsingleton.cpp
        testmodel.h testmodel.cpp
    QML_FILES
        Benchmark.qml
        CreationBenchmark.qml
        V8Benchmark.qml
        Globals.qml
        v8-bench.js
)

target_link_libraries(BenchmarkQmlBench PUBLIC Qt::Gui Qt::Qml Qt::Quick)
