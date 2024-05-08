// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "test262runner.h"

#include <qdebug.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <qfile.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qlibraryinfo.h>
#include <qprocess.h>
#include <qtemporaryfile.h>
#include <qthread.h>

#include "private/qqmlbuiltinfunctions_p.h"
#include "private/qv4arraybuffer_p.h"
#include "private/qv4globalobject_p.h"
#include <QtCore/QLoggingCategory>
#include <private/qv4script_p.h>

using namespace Qt::StringLiterals;

static const char *excludedFeatures[] = {
    "BigInt",
    "class-fields-public",
    "class-fields-private",
    "Promise.prototype.finally",
    "async-iteration",
    "Symbol.asyncIterator",
    "object-rest",
    "object-spread",
    "optional-catch-binding",
    "regexp-dotall",
    "regexp-lookbehind",
    "regexp-named-groups",
    "regexp-unicode-property-escapes",
    "Atomics",
    "SharedArrayBuffer",
    "Array.prototype.flatten",
    "Array.prototype.flatMap",
    "string-trimming",
    "String.prototype.trimEnd",
    "String.prototype.trimStart",
    "numeric-separator-literal",

    // optional features, not supported by us
    "caller",
    nullptr
};

static const char *excludedFilePatterns[] = {
    "realm",
    nullptr
};

QT_BEGIN_NAMESPACE

namespace QV4 {

static ReturnedValue method_detachArrayBuffer(const FunctionObject *f, const Value *, const Value *argv, int argc)
{
    Scope scope(f);
    if (!argc)
        return scope.engine->throwTypeError();
    Scoped<ArrayBuffer> a(scope, argv[0]);
    if (!a)
        return scope.engine->throwTypeError();

    if (a->hasSharedArrayData())
        return scope.engine->throwTypeError();

    a->d()->detachArrayData();

    return Encode::null();
}

void initD262(ExecutionEngine *e)
{
    Scope scope(e);
    ScopedObject d262(scope, e->newObject());

    d262->defineDefaultProperty(QStringLiteral("detachArrayBuffer"), method_detachArrayBuffer, 1);

    ScopedString s(scope, e->newString(QStringLiteral("$262")));
    e->globalObject->put(s, d262);
}

}


Q_DECLARE_LOGGING_CATEGORY(lcJsTest);
Q_LOGGING_CATEGORY(lcJsTest, "qt.v4.ecma262.tests", QtWarningMsg);

Test262Runner::Test262Runner(const QString &command, const QString &dir, const QString &expectationsFile)
    : command(command), testDir(dir), expectationsFile(expectationsFile)
{
    if (testDir.endsWith(QLatin1Char('/')))
        testDir = testDir.chopped(1);
}

Test262Runner::~Test262Runner()
{
    if (threadPool)
        delete threadPool;
}

void Test262Runner::cat()
{
    if (!loadTests())
        return;

    if (testCases.size() != 1)
        qWarning() << "test262 --cat: Ambiguous test case, using" << testCases.begin().key();
    TestData data = getTestData(testCases.begin().value());
    printf("%s", data.content.constData());
}

void Test262Runner::assignTaskOrTerminate(int processIndex)
{
    if (tasks.isEmpty()) {
        sendDone(processIndex);
        return;
    }

    currentTasks[processIndex] = tasks.dequeue();
    TestData &task = currentTasks[processIndex];

    // Sloppy run + maybe strict run later
    if (task.runInSloppyMode) {
        if (task.runInStrictMode)
            task.stillNeedStrictRun = true;
        assignSloppy(processIndex);
        return;
    }

    // Only strict run
    if (task.runInStrictMode) {
        assignStrict(processIndex);
        return;
    }

    // TODO: Start a timer for timeouts?
}

void Test262Runner::assignSloppy(int processIndex)
{
    QProcess &p = *processes[processIndex];
    TestData &task = currentTasks[processIndex];

    QJsonObject json;
    json.insert("mode", "sloppy");
    json.insert("testData", QString::fromUtf8(task.content));
    json.insert("runAsModule", false);
    json.insert("testCasePath", "");
    json.insert("harnessForModules", "");
    p.write(QJsonDocument(json).toJson(QJsonDocument::Compact));
    p.write("\r\n");
}

void Test262Runner::assignStrict(int processIndex)
{
    QProcess &p = *processes[processIndex];
    TestData &task = currentTasks[processIndex];

    QJsonObject json;
    json.insert("mode", "strict");
    QString strictContent = "'use strict';\n" + QString::fromUtf8(task.content);
    json.insert("testData", strictContent);
    json.insert("runAsModule", task.runAsModuleCode);
    json.insert("testCasePath", QFileInfo(testDir + "/test/" + task.test).absoluteFilePath());
    json.insert("harnessForModules", QString::fromUtf8(task.harness));
    p.write(QJsonDocument(json).toJson(QJsonDocument::Compact));
    p.write("\r\n");
}

void Test262Runner::sendDone(int processIndex)
{
    QProcess &p = *processes[processIndex];

    QJsonObject json;
    json.insert("done", true);
    p.write(QJsonDocument(json).toJson(QJsonDocument::Compact));
    p.write("\r\n");
}

void Test262Runner::createProcesses()
{
    const int processCount = QThread::idealThreadCount();
    qDebug() << "Running in parallel with" << processCount << "processes";
    for (int i = 0; i < processCount; ++i) {
        processes.emplace_back(std::make_unique<QProcess>());
        QProcess &p = *processes[i];
        QProcess::connect(&p, &QProcess::started, this, [&, i]() {
            assignTaskOrTerminate(i);
        });

        QProcess::connect(&p, &QIODevice::readyRead, this, [&, i]() {
            QProcess &p = *processes[i];
            QString output;
            while (output.isEmpty())
                output = p.readLine();
            QJsonDocument response = QJsonDocument::fromJson(output.toUtf8());

            TestData &testData(currentTasks[i]);
            auto mode = response["mode"].toString();
            auto state = TestCase::State(response["resultState"].toInt(int(TestCase::State::Fails)));
            auto errorMessage = response["resultErrorMessage"].toString();

            auto &result = mode == "strict" ? testData.strictResult : testData.sloppyResult;
            result = TestCase::Result(state, errorMessage);
            if (testData.negative)
                result.negateResult();

            if (testData.stillNeedStrictRun) {
                testData.stillNeedStrictRun = false;
                assignStrict(i);
            } else {
                addResult(testData);
                assignTaskOrTerminate(i);
            }
        });

        QObject::connect(&p, &QProcess::finished, this,
                         [this, processCount, i](int exitCode, QProcess::ExitStatus status) {
            if (status != QProcess::NormalExit || exitCode != 0) {
                TestData &testData(currentTasks[i]);

                auto &result = testData.stillNeedStrictRun
                        ? testData.sloppyResult
                        : testData.strictResult;
                result = TestCase::Result(
                        TestCase::Crashes,
                        QStringLiteral("Process %1 of %2 exited with a non-normal status")
                                .arg(i).arg(processCount - 1));

                addResult(testData);
            }

            --runningCount;
            if (runningCount == 0)
                loop.exit();
        });

        p.setProgram(QCoreApplication::applicationFilePath());
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert(u"runnerProcess"_s, u"1"_s);
        p.setProcessEnvironment(env);
        ++runningCount;
        p.start();
    }
}

class SingleTest : public QRunnable
{
public:
    SingleTest(Test262Runner *runner, const TestData &data)
        : runner(runner), data(data)
    {}
    void run() override;

    Test262Runner *runner;
    TestData data;
};

TestCase::Result getTestExecutionResult(QV4::ExecutionEngine &vm)
{
    TestCase::State state;
    QString errorMessage;
    if (vm.hasException) {
        state = TestCase::State::Fails;
        QV4::Scope scope(&vm);
        QV4::ScopedValue val(scope, vm.catchException());
        errorMessage = val->toQString();
    } else {
        state = TestCase::State::Passes;
    }
    return TestCase::Result(state, errorMessage);
}

void SingleTest::run()
{
    if (data.runInSloppyMode) {
        QV4::ExecutionEngine vm;
        Test262Runner::executeTest(vm, data.content);
        TestCase::Result ok = getTestExecutionResult(vm);

        if (data.negative)
            ok.negateResult();

        data.sloppyResult = ok;
    } else {
        data.sloppyResult = TestCase::Result(TestCase::Skipped);
    }
    if (data.runInStrictMode) {
        QString testCasePath = QFileInfo(runner->testDirectory() + "/test/" + data.test).absoluteFilePath();
        QByteArray c = "'use strict';\n" + data.content;

        QV4::ExecutionEngine vm;
        Test262Runner::executeTest(vm, c, testCasePath, data.harness, data.runAsModuleCode);
        TestCase::Result ok = getTestExecutionResult(vm);

        if (data.negative)
            ok.negateResult();

        data.strictResult = ok;
    } else {
        data.strictResult = TestCase::Result(TestCase::Skipped);
    }
    runner->addResult(data);
}

void Test262Runner::executeTest(QV4::ExecutionEngine &vm, const QString &testData,
                             const QString &testCasePath, const QString &harnessForModules,
                             bool runAsModule)
{
    QV4::Scope scope(&vm);
    QV4::GlobalExtensions::init(vm.globalObject,
                                QJSEngine::ConsoleExtension | QJSEngine::GarbageCollectionExtension);
    QV4::initD262(&vm);

    if (runAsModule) {
        const QUrl rootModuleUrl = QUrl::fromLocalFile(testCasePath);
        // inject all modules with the harness
        QVector<QUrl> modulesToLoad = { rootModuleUrl };
        while (!modulesToLoad.isEmpty()) {
            QUrl url = modulesToLoad.takeFirst();
            QQmlRefPointer<QV4::ExecutableCompilationUnit> module;

            QFile f(url.toLocalFile());
            if (f.open(QIODevice::ReadOnly)) {
                QByteArray content = harnessForModules.toLocal8Bit() + f.readAll();
                module = vm.compileModule(url.toString(),
                                          QString::fromUtf8(content.constData(),content.size()),
                                          QFileInfo(f).lastModified());
                if (vm.hasException)
                    break;
            } else {
                vm.throwError(QStringLiteral("Could not load module"));
                break;
            }

            const QStringList moduleRequests = module->baseCompilationUnit()->moduleRequests();
            for (const QString &request: moduleRequests) {
                const QUrl absoluteRequest = module->finalUrl().resolved(QUrl(request));
                const auto module = vm.moduleForUrl(absoluteRequest);
                if (module.native == nullptr && module.compiled == nullptr)
                    modulesToLoad << absoluteRequest;
            }
        }

        if (!vm.hasException) {
            const auto rootModule = vm.loadModule(rootModuleUrl);
            if (rootModule.compiled && rootModule.compiled->instantiate())
                rootModule.compiled->evaluate();
        }
    } else {
        QV4::ScopedContext ctx(scope, vm.rootContext());

        QV4::Script script(ctx, QV4::Compiler::ContextType::Global, testData);
        script.parse();

        if (!vm.hasException)
            script.run();
    }
}

void Test262Runner::runWithThreadPool()
{
    threadPool = new QThreadPool();
    threadPool->setStackSize(16*1024*1024);
    qDebug() << "Running in parallel with" << QThread::idealThreadCount() << "threads";

    for (const TestCase &testCase : std::as_const(testCases)) {
        TestData testData = getTestData(testCase);
        if (testData.isExcluded || testData.async)
            continue;
        SingleTest *test = new SingleTest(this, testData);
        threadPool->start(test);
    }

    while (!threadPool->waitForDone(10'000)) {
        if (lcJsTest().isEnabled(QtDebugMsg)) {
            // heartbeat, only needed when there is no other debug output
            qDebug("test262: in progress...");
        }
    }
}

bool Test262Runner::run()
{
    if (!loadTests())
        return false;

    if (flags & ForceJIT)
        qputenv("QV4_JIT_CALL_THRESHOLD", QByteArray("0"));
    else if (flags & ForceBytecode)
        qputenv("QV4_FORCE_INTERPRETER", QByteArray("1"));

    if (flags & WithTestExpectations)
        loadTestExpectations();

    for (auto it = testCases.constBegin(); it != testCases.constEnd(); ++it) {
        auto c = it.value();
        if (!c.skipTestCase) {
            TestData data = getTestData(c);
            if (data.isExcluded || data.async)
                continue;

            tasks.append(data);
        }
    }

    if (command.isEmpty()) {
#if QT_CONFIG(process)
        createProcesses();
        loop.exec();
#else
        runWithThreadPool();
#endif
    } else {
        runAsExternalTests();
    }

    const bool testsOk = report();

    if (flags & WriteTestExpectations)
        writeTestExpectations();
    else if (flags & UpdateTestExpectations)
        updateTestExpectations();

    return testsOk;
}

bool Test262Runner::report()
{
    qDebug() << "Test execution summary:";
    qDebug() << "    Executed" << testCases.size() << "test cases.";
    QStringList crashes;
    QStringList unexpectedFailures;
    QStringList unexpectedPasses;
    for (auto it = testCases.constBegin(); it != testCases.constEnd(); ++it) {
        const auto c = it.value();
        if (c.strictResult.state == c.strictExpectation.state
            && c.sloppyResult.state == c.sloppyExpectation.state)
            continue;
        auto report = [&](const TestCase::Result &expected, const TestCase::Result &result, const char *s) {
            if (result.state == TestCase::Crashes)
                crashes << (it.key() + " crashed in " + s + " mode");
            if (result.state == TestCase::Fails && expected.state == TestCase::Passes)
                unexpectedFailures << (it.key() + " failed in " + s
                                       + " mode with error message: " + result.errorMessage);
            if (result.state == TestCase::Passes && expected.state == TestCase::Fails)
                unexpectedPasses << (it.key() + " unexpectedly passed in " + s + " mode");
        };
        report(c.strictExpectation, c.strictResult, "strict");
        report(c.sloppyExpectation, c.sloppyResult, "sloppy");
    }
    if (!crashes.isEmpty()) {
        qDebug() << "    Encountered" << crashes.size() << "crashes in the following files:";
        for (const QString &f : std::as_const(crashes))
            qDebug() << "        " << f;
    }
    if (!unexpectedFailures.isEmpty()) {
        qDebug() << "    Encountered" << unexpectedFailures.size() << "unexpected failures in the following files:";
        for (const QString &f : std::as_const(unexpectedFailures))
            qDebug() << "        " << f;
    }
    if (!unexpectedPasses.isEmpty()) {
        qDebug() << "    Encountered" << unexpectedPasses.size() << "unexpected passes in the following files:";
        for (const QString &f : std::as_const(unexpectedPasses))
            qDebug() << "        " << f;
    }
    return crashes.isEmpty() && unexpectedFailures.isEmpty() && unexpectedPasses.isEmpty();
}

bool Test262Runner::loadTests()
{
    QDir dir(testDir + "/test");
    if (!dir.exists()) {
        qWarning() << "Could not load tests," << dir.path() << "does not exist.";
        return false;
    }

    QString annexB = "annexB";
    QString harness = "harness";
    QString intl402 = "intl402";

    int pathlen = dir.path().size() + 1;
    QDirIterator it(dir, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString file = it.next().mid(pathlen);
        if (!file.endsWith(".js"))
            continue;
        if (file.endsWith("_FIXTURE.js"))
            continue;
        if (!filter.isEmpty() && !file.contains(filter))
            continue;
        if (file.startsWith(annexB) || file.startsWith(harness) || file.startsWith(intl402))
            continue;
        const char **excluded = excludedFilePatterns;
        bool skip = false;
        while (*excluded) {
            if (file.contains(QLatin1String(*excluded)))
                skip = true;
            ++excluded;
        }
        if (skip)
            continue;

        testCases.insert(file, TestCase{ file });
    }
    if (testCases.isEmpty()) {
        qWarning() << "No tests to run.";
        return false;
    }

    return true;
}


struct TestExpectationLine {
    TestExpectationLine(const QByteArray &line);
    enum State {
        Fails,
        SloppyFails,
        StrictFails,
        Skip,
        Passes
    } state;
    QString testCase;

    QByteArray toLine() const;
    void update(const TestCase &testCase);

    static TestExpectationLine fromTestCase(const TestCase &testCase);
private:
    TestExpectationLine() = default;
    static State stateFromTestCase(const TestCase &testCase);
};

TestExpectationLine::TestExpectationLine(const QByteArray &line)
{
    int space = line.indexOf(' ');

    testCase = QString::fromUtf8(space > 0 ? line.left(space) : line);
    if (!testCase.endsWith(".js"))
        testCase += ".js";

    state = Fails;
    if (space < 0)
        return;
    QByteArray qualifier = line.mid(space + 1);
    if (qualifier == "skip")
        state = Skip;
    else if (qualifier == "strictFails")
        state = StrictFails;
    else if (qualifier == "sloppyFails")
        state = SloppyFails;
    else if (qualifier == "fails")
        state = Fails;
    else
        qWarning() << "illegal format in TestExpectations, line" << line;
}

QByteArray TestExpectationLine::toLine() const {
    const char *res = nullptr;
    switch (state) {
    case Fails:
        res = " fails\n";
        break;
    case SloppyFails:
        res = " sloppyFails\n";
        break;
    case StrictFails:
        res = " strictFails\n";
        break;
    case Skip:
        res = " skip\n";
        break;
    case Passes:
        // no need for an entry
        return QByteArray();
    }
    QByteArray result = testCase.toUtf8() + res;
    return result;
}

void TestExpectationLine::update(const TestCase &testCase)
{
    Q_ASSERT(testCase.test == this->testCase);

    State resultState = stateFromTestCase(testCase);
    switch (resultState) {
    case Fails:
        // no improvement, don't update
        break;
    case SloppyFails:
        if (state == Fails)
            state = SloppyFails;
        else if (state == StrictFails)
            // we have a regression in sloppy mode, but strict now passes
            state = Passes;
        break;
    case StrictFails:
        if (state == Fails)
            state = StrictFails;
        else if (state == SloppyFails)
            // we have a regression in strict mode, but sloppy now passes
            state = Passes;
        break;
    case Skip:
        Q_ASSERT(state == Skip);
        // nothing to do
        break;
    case Passes:
        state = Passes;
    }
}

TestExpectationLine TestExpectationLine::fromTestCase(const TestCase &testCase)
{
    TestExpectationLine l;
    l.testCase = testCase.test;
    l.state = stateFromTestCase(testCase);
    return l;
}

TestExpectationLine::State TestExpectationLine::stateFromTestCase(const TestCase &testCase)
{
    // keep skipped tests
    if (testCase.skipTestCase)
        return Skip;

    bool strictFails = (testCase.strictResult.state == TestCase::Crashes
                        || testCase.strictResult.state == TestCase::Fails);
    bool sloppyFails = (testCase.sloppyResult.state == TestCase::Crashes
                        || testCase.sloppyResult.state == TestCase::Fails);
    if (strictFails && sloppyFails)
        return Fails;
    if (strictFails)
        return StrictFails;
    if (sloppyFails)
        return SloppyFails;
    return Passes;
}


void Test262Runner::loadTestExpectations()
{
    QFile file(expectationsFile);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Could not open TestExpectations file at" << expectationsFile;
        return;
    }

    while (!file.atEnd()) {
        QByteArray line = file.readLine().trimmed();
        if (line.startsWith('#') || line.isEmpty())
            continue;
        TestExpectationLine expectation(line);
        if (!filter.isEmpty() && !expectation.testCase.contains(filter))
            continue;

        if (!testCases.contains(expectation.testCase))
            qWarning() << "Unknown test case" << expectation.testCase << "in TestExpectations file.";
        //qDebug() << "TestExpectations:" << expectation.testCase << expectation.state;
        TestCase &s = testCases[expectation.testCase];
        switch (expectation.state) {
        case TestExpectationLine::Fails:
            s.strictExpectation.state = TestCase::Fails;
            s.sloppyExpectation.state = TestCase::Fails;
            break;
        case TestExpectationLine::SloppyFails:
            s.strictExpectation.state = TestCase::Passes;
            s.sloppyExpectation.state = TestCase::Fails;
            break;
        case TestExpectationLine::StrictFails:
            s.strictExpectation.state = TestCase::Fails;
            s.sloppyExpectation.state = TestCase::Passes;
            break;
        case TestExpectationLine::Skip:
            s.skipTestCase = true;
            break;
        case TestExpectationLine::Passes:
            Q_UNREACHABLE();
        }
    }
}

void Test262Runner::updateTestExpectations()
{
    QFile file(expectationsFile);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Could not open TestExpectations file at" << expectationsFile;
        return;
    }

    QTemporaryFile updatedExpectations;
    if (!updatedExpectations.open()) {
        qFatal("Could not open temporary TestExpectations file: %s",
               qPrintable(updatedExpectations.errorString()));
    }

    while (!file.atEnd()) {
        QByteArray originalLine = file.readLine();
        QByteArray line = originalLine.trimmed();
        if (line.startsWith('#') || line.isEmpty()) {
            updatedExpectations.write(originalLine);
            continue;
        }

        TestExpectationLine expectation(line);
//        qDebug() << "checking: " << expectation.testCase;
        if (!testCases.contains(expectation.testCase)) {
            updatedExpectations.write(originalLine);
            continue;
        }
        const TestCase &testcase = testCases.value(expectation.testCase);
        expectation.update(testcase);

        line = expectation.toLine();
//        qDebug() << "updated line:" << line;
        updatedExpectations.write(line);
    }
    file.close();
    updatedExpectations.close();
    if (!file.remove())
        qWarning() << "Could not remove old TestExpectations file at" << expectationsFile;
    if (updatedExpectations.copy(file.fileName()))
        qDebug() << "Updated TestExpectations file written!";
    else
        qWarning() << "Could not write new TestExpectations file at" << expectationsFile;
}

void Test262Runner::writeTestExpectations()
{
    QFile file(expectationsFile);

    QTemporaryFile expectations;
    if (!expectations.open()) {
        qFatal("Could not open temporary TestExpectations file: %s",
               qPrintable(expectations.errorString()));
    }

    for (const auto &c : std::as_const(testCases)) {
        TestExpectationLine line = TestExpectationLine::fromTestCase(c);
        expectations.write(line.toLine());
    }

    expectations.close();
    if (file.exists() && !file.remove())
        qWarning() << "Could not remove old TestExpectations file at" << expectationsFile;
    if (expectations.copy(file.fileName()))
        qDebug() << "new TestExpectations file written!";
    else
        qWarning() << "Could not write new TestExpectations file at" << expectationsFile;
}

void Test262Runner::runAsExternalTests()
{
    for (TestData &testData : tasks) {
        auto runTest = [&] (const char *header, TestCase::Result *result) {
            QTemporaryFile tempFile;
            if (!tempFile.open()) {
                qFatal("Could not open temporary test data file: %s",
                       qPrintable(tempFile.errorString()));
            }
            tempFile.write(header);
            tempFile.write(testData.content);
            tempFile.close();

            QProcess process;
            process.start(command, QStringList(tempFile.fileName()));
            if (!process.waitForFinished(-1) || process.error() == QProcess::FailedToStart) {
                qWarning() << "Could not execute" << command;
                *result = TestCase::Result(TestCase::Crashes);
            }
            if (process.exitStatus() != QProcess::NormalExit) {
                *result = TestCase::Result(TestCase::Crashes);
            }
            bool ok = (process.exitCode() == EXIT_SUCCESS);
            if (testData.negative)
                ok = !ok;
            *result = ok ? TestCase::Result(TestCase::Passes)
                         : TestCase::Result(TestCase::Fails, process.readAllStandardError());
        };

        if (testData.runInSloppyMode)
            runTest("", &testData.sloppyResult);
        if (testData.runInStrictMode)
            runTest("'use strict';\n", &testData.strictResult);

        addResult(testData);
    }
}

void Test262Runner::addResult(TestCase result)
{
    {
#if !QT_CONFIG(process)
        QMutexLocker locker(&mutex);
#endif
        Q_ASSERT(result.strictExpectation.state == testCases[result.test].strictExpectation.state);
        Q_ASSERT(result.sloppyExpectation.state == testCases[result.test].sloppyExpectation.state);
        testCases[result.test] = result;
    }

    if (!(flags & Verbose))
        return;

    QString test = result.test;
    if (result.strictResult.state == TestCase::Skipped) {
        ;
    } else if (result.strictResult.state == TestCase::Crashes) {
        qDebug() << "FAIL:" << test << "crashed in strict mode!";
    } else if (result.strictResult.state == TestCase::Fails
               && result.strictExpectation.state == TestCase::Fails) {
        qCDebug(lcJsTest) << "PASS:" << test << "failed in strict mode as expected";
    } else if ((result.strictResult.state == TestCase::Passes)
               == (result.strictExpectation.state == TestCase::Passes)) {
        qCDebug(lcJsTest) << "PASS:" << test << "passed in strict mode";
    } else if (!(result.strictExpectation.state == TestCase::Fails)) {
        qDebug() << "FAIL:" << test << "failed in strict mode with error message:\n"
                 << result.strictResult.errorMessage;
    } else {
        qDebug() << "XPASS:" << test << "unexpectedly passed in strict mode";
    }

    if (result.sloppyResult.state == TestCase::Skipped) {
        ;
    } else if (result.sloppyResult.state == TestCase::Crashes) {
        qDebug() << "FAIL:" << test << "crashed in sloppy mode!";
    } else if (result.sloppyResult.state == TestCase::Fails
               && result.sloppyExpectation.state == TestCase::Fails) {
        qCDebug(lcJsTest) << "PASS:" << test << "failed in sloppy mode as expected";
    } else if ((result.sloppyResult.state == TestCase::Passes)
               == (result.sloppyExpectation.state == TestCase::Passes)) {
        qCDebug(lcJsTest) << "PASS:" << test << "passed in sloppy mode";
    } else if (!(result.sloppyExpectation.state == TestCase::Fails)) {
        qDebug() << "FAIL:" << test << "failed in sloppy mode with error message:\n"
                 << result.sloppyResult.errorMessage;
    } else {
        qDebug() << "XPASS:" << test << "unexpectedly passed in sloppy mode";
    }
}

TestData Test262Runner::getTestData(const TestCase &testCase)
{
    QFile testFile(testDir + "/test/" + testCase.test);
    if (!testFile.open(QFile::ReadOnly)) {
        qWarning() << "wrong test file" << testCase.test;
        exit(1);
    }
    QByteArray content = testFile.readAll();
    content.replace(QByteArrayLiteral("\r\n"), "\n");

    qCDebug(lcJsTest) << "parsing test file" << testCase.test;

    TestData data(testCase);
    parseYaml(content, &data);

    data.harness += harness("assert.js");
    data.harness += harness("sta.js");

    for (QByteArray inc : std::as_const(data.includes)) {
        inc = inc.trimmed();
        data.harness += harness(inc);
    }

    if (data.async)
        data.harness += harness("doneprintHandle.js");

    data.content = data.harness + content;

    return data;
}

struct YamlSection {
    YamlSection(const QByteArray &yaml, const char *sectionName);

    bool contains(const char *keyword) const;
    QList<QByteArray> keywords() const;

    QByteArray yaml;
    int start = -1;
    int length = 0;
    bool shortSection = false;
};

YamlSection::YamlSection(const QByteArray &yaml, const char *sectionName)
    : yaml(yaml)
{
    start = yaml.indexOf(sectionName);
    if (start < 0)
        return;
    start += static_cast<int>(strlen(sectionName));
    int end = yaml.indexOf('\n', start + 1);
    if (end < 0)
        end = yaml.size();

    int s = yaml.indexOf('[', start);
    if (s > 0 && s < end) {
        shortSection = true;
        start = s + 1;
        end = yaml.indexOf(']', s);
    } else {
        while (end < yaml.size() - 1 && yaml.at(end + 1) == ' ')
            end = yaml.indexOf('\n', end + 1);
    }
    length = end - start;
}

bool YamlSection::contains(const char *keyword) const
{
    if (start < 0)
        return false;
    int idx = yaml.indexOf(keyword, start);
    if (idx >= start && idx < start + length)
        return true;
    return false;
}

QList<QByteArray> YamlSection::keywords() const
{
    if (start < 0)
        return QList<QByteArray>();

    QByteArray content = yaml.mid(start, length);
    QList<QByteArray> keywords;
    if (shortSection) {
        keywords = content.split(',');
    } else {
        const QList<QByteArray> list = content.split('\n');
        for (const QByteArray &l : list) {
            int i = 0;
            while (i < l.size() && (l.at(i) == ' ' || l.at(i) == '-'))
                ++i;
            QByteArray entry = l.mid(i);
            if (!entry.isEmpty())
                keywords.append(entry);
        }
    }
//    qDebug() << "keywords:" << keywords;
    return keywords;
}


void Test262Runner::parseYaml(const QByteArray &content, TestData *data)
{
    int start = content.indexOf("/*---");
    if (start < 0)
        return;
    start += sizeof("/*---");

    int end = content.indexOf("---*/");
    if (end < 0)
        return;

    QByteArray yaml = content.mid(start, end - start);

    if (yaml.contains("negative:"))
        data->negative = true;

    YamlSection flags(yaml, "flags:");
    data->runInSloppyMode = !flags.contains("onlyStrict");
    data->runInStrictMode = !flags.contains("noStrict") && !flags.contains("raw");
    data->runAsModuleCode = flags.contains("module");
    data->async = flags.contains("async");

    if (data->runAsModuleCode) {
        data->runInStrictMode = true;
        data->runInSloppyMode = false;
    }

    YamlSection includes(yaml, "includes:");
    data->includes = includes.keywords();

    YamlSection features = YamlSection(yaml, "features:");

    const char **f = excludedFeatures;
    while (*f) {
        if (features.contains(*f)) {
            data->isExcluded = true;
            break;
        }
        ++f;
    }

//    qDebug() << "Yaml:\n" << yaml;
}

QByteArray Test262Runner::harness(const QByteArray &name)
{
    if (harnessFiles.contains(name))
        return harnessFiles.value(name);

    QFile h(testDir + QLatin1String("/harness/") + name);
    if (!h.open(QFile::ReadOnly)) {
        qWarning() << "Illegal test harness file" << name;
        exit(1);
    }

    QByteArray content = h.readAll();
    harnessFiles.insert(name, content);
    return content;
}

QT_END_NAMESPACE
