// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QFileInfo>
#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibraryInfo>
#include <QProcess>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtTest/QtTest>

#include "test262runner.h"
#include "private/qqmlbuiltinfunctions_p.h"
#include "private/qv4arraybuffer_p.h"
#include "private/qv4globalobject_p.h"
#include "private/qv4script_p.h"

#include <stdio.h>

class tst_EcmaScriptTests : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_EcmaScriptTests()
        : QQmlDataTest(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::DoNotFailOnWarnings, "test262")
    {
        if (!qEnvironmentVariableIsEmpty("QTEST_FUNCTION_TIMEOUT"))
            return;
#ifdef Q_OS_ANDROID
    qputenv("QTEST_FUNCTION_TIMEOUT", "1800000"); // 30 minutes for android
#else
    qputenv("QTEST_FUNCTION_TIMEOUT", "900000");  // 15 minutes for everything else
#endif
    }

private slots:
    void initTestCase() final;
    void cleanupTestCase();
    void runInterpreted();
    void runJitted();

private:
    static QLoggingCategory::CategoryFilter priorFilter;
    static void filterCategories(QLoggingCategory *category);
};
QLoggingCategory::CategoryFilter tst_EcmaScriptTests::priorFilter = nullptr;

static inline bool isNoise(QByteArrayView name)
{
#ifdef QT_V4_WANT_ES262_WARNINGS
    return false;
#else
    const QByteArrayView noisy("qt.qml.usedbeforedeclared");
    return name.startsWith(noisy) && (name.size() <= noisy.size() || name[noisy.size()] == '.');
#endif
}

void tst_EcmaScriptTests::filterCategories(QLoggingCategory *category)
{
    if (priorFilter)
        priorFilter(category);

    if (isNoise(category->categoryName())) {
        category->setEnabled(QtDebugMsg, false);
        category->setEnabled(QtWarningMsg, false);
    }
}

void tst_EcmaScriptTests::initTestCase()
{
    QQmlDataTest::initTestCase();
    /* Suppress lcQmlCompiler's "qt.qml.usedbeforedeclared" warnings; we aren't in a
       position to fix test262's many warnings and they flood messages so we
       didn't get to see actual failures unless we passed -maxwarnings with a
       huge value on the command-line (resulting in huge log output).
    */
    priorFilter = QLoggingCategory::installFilter(filterCategories);
}

void tst_EcmaScriptTests::cleanupTestCase()
{
    QLoggingCategory::installFilter(priorFilter);
}

void tst_EcmaScriptTests::runInterpreted()
{
    Test262Runner runner(QString(), dataDirectory(), directory() + QStringLiteral("/TestExpectations"));
    runner.setFlags(Test262Runner::ForceBytecode
                    | Test262Runner::WithTestExpectations
                    | Test262Runner::Parallel
                    | Test262Runner::Verbose);
    bool result = runner.run();
    QVERIFY(result);
}

void tst_EcmaScriptTests::runJitted()
{
    Test262Runner runner(QString(), dataDirectory(), directory() + QStringLiteral("/TestExpectations"));
    runner.setFlags(Test262Runner::ForceJIT
                    | Test262Runner::WithTestExpectations
                    | Test262Runner::Parallel
                    | Test262Runner::Verbose);
    bool result = runner.run();
    QVERIFY(result);
}

//// v RUNNER PROCESS MODE v ////

void readInput(bool &done, QString &mode, QString &testData, QString &testCasePath,
               QString &harnessForModules, bool &runAsModule)
{
    QTextStream in(stdin);
    QString input;
    while (input.isEmpty())
        input = in.readLine();

    QJsonDocument json = QJsonDocument::fromJson(input.toUtf8());
    done = json["done"].toBool(false);
    mode = json["mode"].toString();
    testData = json["testData"].toString();
    testCasePath = json["testCasePath"].toString();
    harnessForModules = json["harnessForModules"].toString();
    runAsModule = json["runAsModule"].toBool(false);
}

void printResult(QV4::ExecutionEngine &vm, const QString &mode)
{
    QJsonObject result;
    result.insert("mode", mode);
    if (vm.hasException) {
        QV4::Scope scope(&vm);
        QV4::ScopedValue val(scope, vm.catchException());

        result.insert("resultState", int(TestCase::State::Fails));
        result.insert("resultErrorMessage", val->toQString());
    } else {
        result.insert("resultState", int(TestCase::State::Passes));
    }

    QTextStream(stdout) << QJsonDocument(result).toJson(QJsonDocument::Compact) << "\r\n";
}

void doRunnerProcess()
{
    bool done = false;
    QString mode;
    QString testData;
    QString testCasePath;
    QString harnessForModules;
    bool runAsModule = false;

    while (!done) {
        QV4::ExecutionEngine vm;
        readInput(done, mode, testData, testCasePath, harnessForModules, runAsModule);
        if (done)
            break;
        Test262Runner::executeTest(vm, testData, testCasePath, harnessForModules, runAsModule);
        printResult(vm, mode);
    }
}

//// ^ RUNNER PROCESS MODE ^ ////

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (qEnvironmentVariableIntValue("runnerProcess") == 1) {
        doRunnerProcess();
    } else {
        tst_EcmaScriptTests tc;
        QTEST_SET_MAIN_SOURCE_PATH
        return QTest::qExec(&tc, argc, argv);
    }
}

#include "tst_ecmascripttests.moc"
