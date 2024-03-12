// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TEST262RUNNER_H
#define TEST262RUNNER_H

#include <qeventloop.h>
#include <qmap.h>
#include <qmutex.h>
#include <qprocess.h>
#include <qqueue.h>
#include <qset.h>
#include <qthreadpool.h>

QT_BEGIN_NAMESPACE

namespace QV4 {
struct ExecutionEngine;
void initD262(ExecutionEngine *e);
}

struct TestCase {
    TestCase() = default;
    TestCase(const QString &test)
        : test(test) {}

    enum State { Skipped, Passes, Fails, Crashes };

    struct Result
    {
        State state;
        QString errorMessage;

        Result(State state, QString errorMessage = "")
            : state(state), errorMessage(errorMessage) { }

        void negateResult()
        {
            switch (state) {
            case TestCase::Passes:
                state = TestCase::Fails;
                break;
            case TestCase::Fails:
                state = TestCase::Passes;
                break;
            case TestCase::Skipped:
            case TestCase::Crashes:
                break;
            }
        }
    };

    Result strictExpectation = Result(Passes);
    Result sloppyExpectation = Result(Passes);
    Result strictResult = Result(Skipped);
    Result sloppyResult = Result(Skipped);
    bool skipTestCase = false;
    bool stillNeedStrictRun = false;

    QString test;
};

struct TestData : TestCase {
    TestData() = default;
    TestData(const TestCase &testCase)
        : TestCase(testCase) {}
    // flags
    bool negative = false;
    bool runInStrictMode = true;
    bool runInSloppyMode = true;
    bool runAsModuleCode = false;
    bool async = false;

    bool isExcluded = false;

    QList<QByteArray> includes;

    QByteArray harness;
    QByteArray content;
};

class SingleTest;

class Test262Runner : public QObject
{
    Q_OBJECT

public:
    Test262Runner(const QString &command, const QString &testDir, const QString &expectationsFile);
    ~Test262Runner();

    enum Mode {
        Sloppy = 0,
        Strict = 1
    };

    enum Flags {
        Verbose = 0x1,
        Parallel = 0x2,
        ForceBytecode = 0x4,
        ForceJIT = 0x8,
        WithTestExpectations = 0x10,
        UpdateTestExpectations = 0x20,
        WriteTestExpectations = 0x40,
    };
    void setFlags(int f) { flags = f; }

    void setFilter(const QString &f) { filter = f; }

    void cat();
    bool run();

    bool report();
    QString testDirectory() const { return testDir; }

    static void executeTest(QV4::ExecutionEngine &vm, const QString &testData,
                            const QString &testCasePath = QString(),
                            const QString &harnessForModules = QString(),
                            bool runAsModule = false);

private:
    friend class SingleTest;
    bool loadTests();
    void loadTestExpectations();
    void updateTestExpectations();
    void writeTestExpectations();

    void runWithThreadPool();

    void runAsExternalTests();
    void createProcesses();
    void assignTaskOrTerminate(int processIndex);
    void assignSloppy(int processIndex);
    void assignStrict(int processIndex);
    void sendDone(int processIndex);
    QString readUntilNull(QProcess &p);

    TestData getTestData(const TestCase &testCase);
    void parseYaml(const QByteArray &content, TestData *data);

    QByteArray harness(const QByteArray &name);

    void addResult(TestCase result);

    QString command;
    QString testDir;
    QString expectationsFile;
    int flags = 0;

    QString filter;

    QMap<QString, TestCase> testCases;
    QHash<QByteArray, QByteArray> harnessFiles;

    QThreadPool *threadPool = nullptr;
    QMutex mutex;

    QEventLoop loop;
    std::vector<std::unique_ptr<QProcess>> processes;
    int runningCount = 0;
    QQueue<TestData> tasks;
    QHash<int, TestData> currentTasks;
};

QT_END_NAMESPACE

#endif
