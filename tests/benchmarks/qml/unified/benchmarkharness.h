// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef BENCHMARKHARNESS_H
#define BENCHMARKHARNESS_H

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qtenvironmentvariables.h>

#include <functional>

// Pin the QV4 JIT call-count threshold to a deterministic value before any engine is
// created (it is read once, when the first ExecutionEngine is built). The default (3)
// would let the JIT compile a binding partway through a warm run() and split the
// sample; forcing 0 makes the JIT kick in on the very first call, so "run 1st" absorbs
// the JIT-compile and "run" measures the steady JITted state. An existing
// QV4_JIT_CALL_THRESHOLD (or QV4_FORCE_INTERPRETER) is honoured, so a pure-interpreter
// variant can be requested from the environment without touching this code.
inline void pinJitThreshold()
{
    if (!qEnvironmentVariableIsSet("QV4_JIT_CALL_THRESHOLD"))
        qputenv("QV4_JIT_CALL_THRESHOLD", "0");
}

// Reusable benchmark body shared by every mode executable (compile-from-source,
// .qmlc loading, AOT). A mode executable constructs one of these with the qrc
// directory holding its documents and the matching module-URI prefix, then hands
// it to QTest::qExec. The disk-cache behaviour that distinguishes the modes is
// selected once per process via the environment, in each executable's main().
//
// Phases (a document opts in by what it is):
//   compile  - cold load of the document into a fresh engine (untimed reset), via
//              QQmlComponent::loadFromModule(). What that costs depends only on the
//              mode's disk-cache environment and which bundle is linked: recompile
//              from source, read the run-time .qmlc, or map the baked-in AOT unit.
//   create   - instantiate the root object on a pre-heated engine.
//   run 1st  - one cold invocation of the document's central operation (driving the
//              `t` tick, a property round-trip, or a method call), which populates
//              the QV4 inline-cache lookups. Timed once, separately.
//   run      - the warm steady state of that same operation (QBENCHMARK). With the
//              lookups already populated, ahead-of-time-compiled bindings show their
//              advantage and the measurement settles quickly.
// Documents whose only workload is creation define no operation and are skipped for
// run 1st / run.
class BenchmarkRunner : public QObject
{
    Q_OBJECT
public:
    // resourceRoot: qrc directory to enumerate, e.g. ":/qt/qml/Bundle/src".
    // moduleUriPrefix: URI prefix the groups live under, e.g. "Bundle.src"; the
    // group directory name is appended to form each document's module URI.
    BenchmarkRunner(QString resourceRoot, QString moduleUriPrefix, QObject *parent = nullptr);

    // Builds the per-iteration operation for a freshly created `object` (drive the
    // `t` tick, a property round-trip, or a method call), or an empty function if the
    // document is creation-only. Static so other suites (the exec A/B) build the exact
    // same operation from the one invocation table instead of duplicating it.
    static std::function<void()> operationFor(const QString &row, QObject *object);

private slots:
    void initTestCase();

    // Runs after every data row (every document). Clears the global QQmlMetaType registry so
    // each document starts from the same baseline. Without this the registry accumulates every
    // composite type from every prior document for the life of the process, and the per-load
    // type (un)registration — QQmlMetaType::removeQQmlTypePrivate, a linear scan of the global
    // multihashes, reached on the main thread from QQmlComponent::loadFromModule — grows with it,
    // so a document's measured compile cost would depend on its position in the run.
    void cleanup();

    void compile_data();
    void compile();

    void create_data();
    void create();

    // The document's central operation, split in two: "run 1st" times a single cold
    // invocation (which populates the QV4 inline-cache lookups), "run" (more) times
    // the warm steady state with QBENCHMARK. Separating the cold first call keeps the
    // warm measurement consistent so QBENCHMARK settles quickly. Documents whose only
    // workload is creation define no operation and are skipped for both.
    void runfirst_data();
    void runfirst();

    void run_data();
    void run();

private:
    struct ModuleType
    {
        QString uri;
        QString type;
    };
    ModuleType moduleTypeFor(const QString &docPath) const;
    void addDocRows();
    void writeBreakdown(const char *phase, const QString &row, qint64 totalNs) const;

    QString m_root;
    QString m_uriPrefix;
    QString m_breakdownPath;
    QStringList m_docs;
};

#endif // BENCHMARKHARNESS_H
