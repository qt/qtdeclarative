// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef COLDBENCHMARK_H
#define COLDBENCHMARK_H

#include <QtQml/qqmlengine.h>

#include <QtCore/qelapsedtimer.h>

#include <QtTest/qtest.h>
#include <QtTest/qbenchmark.h>

#include <algorithm>
#include <limits>
#include <memory>

QT_BEGIN_NAMESPACE

// A clean-slate QML engine for the compile/load measurement.
//
// reset() throws the engine away and builds a fresh one. Each fresh QQmlEngine
// has its own empty QQmlTypeLoader cache and its own empty QV4 list of
// ExecutableCompilationUnit, so the next load genuinely redoes the per-engine
// work: recompile from source, read the run-time .qmlc, or map the baked-in AOT
// unit, depending on the mode.
//
// Deliberately, reset() does NOT call qmlClearTypeRegistrations(): wiping the global
// QQmlMetaType registry between the timed rounds would unregister the C++ modules
// (QtQuick, ...) and force a full re-registration on the next import — inside the timed
// region. That re-registration costs far more than loading the document itself and is
// identical work in every mode, so it would swamp and distort the comparison (in
// particular it makes .qmlc loading look artificially slow next to AOT). The registry IS
// cleared once *between documents* (BenchmarkRunner::cleanup(), outside any timed region)
// so it does not accumulate every prior document's types for the life of the process.
//
// Composite units (compiled from source or read from .qmlc) ARE interned in the
// global QQmlMetaType (registerInternalCompositeType), but that entry is owned by
// the loading engine's per-engine QQmlTypeLoader: ~QQmlTypeLoader::clearCache()
// unregisters it when no live object still holds it. So the destroy-before-create
// order below is load-bearing — tearing the old engine down first releases the
// previous iteration's units, so the fresh engine genuinely re-loads (re-parse for
// source, re-read for .qmlc) instead of getting a warm global hit. (AOT units come
// from the statically baked CachedQmlUnit, which is not released this way; a fresh
// engine re-discovers it and only rebuilds the per-engine ExecutableCompilationUnit
// — exactly the AOT load cost we want to measure.)
class ColdEngine
{
public:
    void reset()
    {
        m_engine.reset();                          // (1) destroy old: releases its composite units
        m_engine = std::make_unique<QQmlEngine>(); // (2) only then: fresh empty type loader + CU list
    }

    QQmlEngine *engine() const { return m_engine.get(); }

private:
    std::unique_ptr<QQmlEngine> m_engine;
};

// Runs `iterations` rounds of: untimed `before`, timed `operation`, untimed
// `after`. Only `operation` is measured; the cost of `before`/`after` is excluded
// by construction.
//
// This is the whole reason we cannot use the QBENCHMARK macro for these modes:
// QBENCHMARK drives its own iteration loop and times the entire body, so any
// per-iteration setup or teardown placed inside it would be measured too. We take
// over the loop, time only the operation with a nanosecond timer, and feed the
// result back into Qt Test's reporting via setBenchmarkResult(). The minimum
// across iterations is the most robust estimator against scheduler noise.
template <typename Before, typename Operation, typename After>
void runTimed(int iterations, Before &&before, Operation &&operation, After &&after)
{
    qint64 best = std::numeric_limits<qint64>::max();
    for (int i = 0; i < iterations; ++i) {
        before();
        QElapsedTimer timer;
        timer.start();
        operation();
        best = std::min(best, timer.nsecsElapsed());
        after();
    }
    QTest::setBenchmarkResult(qreal(best), QTest::WalltimeNanoseconds);
}

// Cold compile/load: clean slate (untimed) before each timed operation.
template <typename Reset, typename Operation>
void runColdBenchmark(int iterations, Reset &&reset, Operation &&operation)
{
    runTimed(iterations, std::forward<Reset>(reset), std::forward<Operation>(operation), [] {});
}

QT_END_NAMESPACE

#endif // COLDBENCHMARK_H
