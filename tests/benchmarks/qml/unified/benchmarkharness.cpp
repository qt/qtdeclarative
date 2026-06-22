// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "benchmarkharness.h"
#include "coldbenchmark.h"

#include <QtCore/private/qtcore-config_p.h>

#include <QtQml/qqml.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qhash.h>
#include <QtCore/qmetaobject.h>

#include <QtTest/qtest.h>

#include <memory>

using namespace Qt::StringLiterals;

namespace {
// compile rebuilds the engine and re-loads from cold each round; a handful of rounds
// and the minimum is enough. The count is overridable via the environment (it is the
// one repetition count QTest's own -iterations cannot reach, since compile is not a
// QBENCHMARK). create/run/run-1st use QBENCHMARK / QBENCHMARK_ONCE, whose iteration
// counts are controlled by the standard QTest -iterations/-minimumvalue arguments.
int compileRounds()
{
    bool ok = false;
    const int n = qEnvironmentVariableIntValue("QMLBENCH_COMPILE_ROUNDS", &ok);
    return (ok && n > 0) ? n : 5;
}

// Documents that define a central operation for the run phases: one repeatable action
// that drives a single AOT-compilable binding, change handler, or function. Either a
// property round-trip (write the int `in`, read `out`) or a method invocation
// (`method` signature, optionally with one int `arg`). Keyed by row name (group/Doc.qml).
struct Invocation
{
    const char *in = nullptr;
    const char *out = nullptr;
    const char *method = nullptr;
    int arg = 0;
    bool hasArg = false;
};
const QHash<QString, Invocation> &invocations()
{
    static const QHash<QString, Invocation> table = {
        // Property round-trips (write int -> binding/handler re-runs -> read int).
        { u"cppcodegen/IdAccess.qml"_s, { "x", "y" } },
        { u"cppcodegen/ValueTypeProperty.qml"_s, { "inR", "outR" } },
        { u"cppcodegen/FuncWithParams.qml"_s, { "a", "b" } },
        { u"cppcodegen/StoreNameSloppy.qml"_s, { "input", "output" } },
        { u"cppcodegen/ValueTypeList.qml"_s, { "a", "o" } },
        { u"cppcodegen/DynamicOne.qml"_s, { "dynamicWidth", "height" } },
        { u"cppcodegen/DynamicTwo.qml"_s, { "dynamicWidth", "dynamicHeight" } },
        { u"cppcodegen/DynamicThree.qml"_s, { "dynamicWidth", "widthSignaledProperty" } },
        { u"cppcodegen/DynamicFour.qml"_s, { "dynamicWidth", "dynamicHeight" } },
        // Method invocations (the function body is what the AOT compiler turns to C++).
        { u"cppcodegen/Methods.qml"_s, { nullptr, nullptr, "sumTyped(int)", 100, true } },
        { u"cppcodegen/CallFrameSetup.qml"_s, { nullptr, nullptr, "noop()" } },
        // Real example: writing the page width cascades through ratio -> smallSize/
        // bigSize/elementSpacing -> the size bindings of every positioned rectangle,
        // i.e. a large fan-out of typed numeric bindings the AOT compiler can emit.
        { u"examples/PositionersTransitions.qml"_s, { "width", "bigSize" } },
        // DeltaBlue constraint solver. Only the solves that rebuild their whole state
        // per call get a run op, so the warm-repeated run phase stays meaningful:
        // ChainTest re-plans and re-executes its chain each call, and Main builds a
        // fresh planner/variable graph on every deltaBlue(). ProjectionTest and Main2
        // permanently mutate the shared scale/offset variables (and add/remove the edit
        // constraint), so they are not repeatable; they remain creation-only documents
        // (their solves still run once at creation, and the projection solve is also
        // exercised inside Main; the pure-JS DeltaBlue run is covered by V8DeltaBlue).
        { u"deltablue/ChainTest.qml"_s, { nullptr, nullptr, "run()" } },
        { u"deltablue/Main.qml"_s, { nullptr, nullptr, "deltaBlue()" } },
    };
    // AnchorsFill, IdProperty, NonNotifyable, Easing have no per-iteration operation
    // (their whole benchmark is creation), so they carry no entry and are skipped for
    // the run phases — their run cells stay empty rather than recycling create.
    return table;
}
} // namespace

BenchmarkRunner::BenchmarkRunner(QString resourceRoot, QString moduleUriPrefix, QObject *parent)
    : QObject(parent),
      m_root(std::move(resourceRoot)),
      m_uriPrefix(std::move(moduleUriPrefix)),
      m_breakdownPath(qEnvironmentVariable("QMLBENCH_BREAKDOWN"))
{
}

void BenchmarkRunner::initTestCase()
{
    QDirIterator it(m_root, { u"*.qml"_s }, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        m_docs.append(it.next());
    m_docs.sort();
    QVERIFY2(!m_docs.isEmpty(), qPrintable(u"No QML documents found under "_s + m_root));
}

void BenchmarkRunner::cleanup()
{
    // The test body's engines are gone, but some QML objects (e.g. Qt Quick Controls popups)
    // may have been scheduled for deferred deletion. They must be destroyed *before* the
    // registry is wiped — otherwise their now-freed metaobjects are dereferenced when the
    // DeferredDelete events are finally delivered (a use-after-free in ~QObject). Drain them.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // Now reset the global registry to its built-in baseline (the imported C++ modules
    // re-register lazily on the next document's untimed warm-up load), keeping every document's
    // measured (un)registration cost independent of how many documents ran before it. See the
    // header.
    qmlClearTypeRegistrations();
}

// A document at ":/qt/qml/Bundle/src/<group>/<Type>.qml" lives in module
// "<prefix>.<group>" as type "<Type>". Nested group directories collapse to
// dotted URI components.
BenchmarkRunner::ModuleType BenchmarkRunner::moduleTypeFor(const QString &docPath) const
{
    const QString relative = docPath.sliced(m_root.size() + 1); // strip "<root>/"
    const qsizetype lastSlash = relative.lastIndexOf(u'/');
    const QString groupPath = relative.first(lastSlash);
    const QString type = QFileInfo(relative).completeBaseName();
    return { m_uriPrefix + u'.' + QString(groupPath).replace(u'/', u'.'), type };
}

void BenchmarkRunner::addDocRows()
{
    QTest::addColumn<QString>("path");
    const qsizetype prefix = m_root.size() + 1; // strip "<root>/"
    for (const QString &doc : std::as_const(m_docs))
        QTest::newRow(qPrintable(doc.sliced(prefix))) << doc;
}

void BenchmarkRunner::compile_data()
{
    addDocRows();
}

void BenchmarkRunner::compile()
{
    QFETCH(QString, path);
    const ModuleType mt = moduleTypeFor(path);

    const auto load = [&](QQmlEngine *engine) {
        QQmlComponent component(engine);
        component.loadFromModule(mt.uri, mt.type);
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    };

    // Pre-heat (untimed): load once so that in the .qmlc mode the disk cache file
    // is written before we measure. In compile-from-source mode the disk cache is
    // disabled, so this is just a discarded warm-up; in AOT mode the baked unit is
    // used regardless. Each timed iteration then loads into a cold engine.
    {
        QQmlEngine warm;
        load(&warm);
    }

    ColdEngine cold;
    runColdBenchmark(compileRounds(), [&] { cold.reset(); }, [&] { load(cold.engine()); });
}

void BenchmarkRunner::create_data()
{
    addDocRows();
}

void BenchmarkRunner::create()
{
    QFETCH(QString, path);
    const ModuleType mt = moduleTypeFor(path);

    // Pre-heated engine: load the type once (warm), then measure instantiation. The
    // iteration count is QBENCHMARK's, tunable via -iterations/-minimumvalue.
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadFromModule(mt.uri, mt.type);
    if (!component.isReady())
        QSKIP(qPrintable(component.errorString()));

    // Some documents compile but cannot be instantiated standalone: composite
    // singletons, and building-block types that declare required properties (e.g.
    // the DeltaBlue constraints). Their compile cost is still measured; create/run
    // are skipped rather than failing the benchmark.
    {
        std::unique_ptr<QObject> probe(component.create());
        if (!probe)
            QSKIP(qPrintable(component.errorString()));
    }

    QBENCHMARK {
        std::unique_ptr<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
    }
}

std::function<void()> BenchmarkRunner::operationFor(const QString &row, QObject *object)
{
    const QMetaObject *mo = object->metaObject();

    // Tick-driven document: each `t` change re-runs the dependent bindings. Cache the
    // QMetaProperty so the per-iteration write resolves "t" once, not on every call.
    if (const int tIndex = mo->indexOfProperty("t"); tIndex >= 0) {
        const QMetaProperty t = mo->property(tIndex);
        auto tick = std::make_shared<qreal>(0.0);
        return [object, t, tick] { t.write(object, *tick += 1.0); };
    }

    const auto it = invocations().constFind(row);
    if (it == invocations().cend())
        return {};

    if (it->method) {
        const QMetaMethod method = mo->method(mo->indexOfMethod(it->method));
        if (!method.isValid())
            return {};
        if (it->hasArg) {
            const int arg = it->arg;
            return [object, method, arg] {
                method.invoke(object, Qt::DirectConnection, Q_ARG(int, arg));
            };
        }
        return [object, method] { method.invoke(object, Qt::DirectConnection); };
    }

    const QMetaProperty in = mo->property(mo->indexOfProperty(it->in));
    const QMetaProperty out = mo->property(mo->indexOfProperty(it->out));
    if (!in.isValid() || !out.isValid())
        return {};
    auto counter = std::make_shared<int>(0);
    return [object, in, out, counter] {
        in.write(object, ++*counter);
        out.read(object);
    };
}

void BenchmarkRunner::runfirst_data()
{
    addDocRows();
}

void BenchmarkRunner::runfirst()
{
    QFETCH(QString, path);
    const QString row = path.sliced(m_root.size() + 1);
    const ModuleType mt = moduleTypeFor(path);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadFromModule(mt.uri, mt.type);
    if (!component.isReady())
        QSKIP(qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    if (!object)
        QSKIP(qPrintable(component.errorString()));

    const std::function<void()> op = operationFor(row, object.get());
    if (!op)
        QSKIP("creation-only document: no run operation");

    // A single cold invocation in this fresh engine: the inline-cache lookups are
    // unpopulated, so this captures the first-call cost (lookup population included).
    QBENCHMARK_ONCE {
        op();
    }
}

void BenchmarkRunner::run_data()
{
    addDocRows();
}

void BenchmarkRunner::run()
{
    QFETCH(QString, path);
    const QString row = path.sliced(m_root.size() + 1);
    const ModuleType mt = moduleTypeFor(path);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadFromModule(mt.uri, mt.type);
    if (!component.isReady())
        QSKIP(qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    if (!object)
        QSKIP(qPrintable(component.errorString()));

    const std::function<void()> op = operationFor(row, object.get());
    if (!op)
        QSKIP("creation-only document: no run operation");

    // Warm up once (the cold first-call cost is reported separately by run 1st), then
    // measure the steady state. With the lookups already populated the measurements
    // are consistent, so QBENCHMARK settles quickly; its iteration count is governed
    // by the standard QTest -iterations/-minimumvalue arguments.
    op();
    QBENCHMARK {
        op();
    }
}