// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QProcess>
#include <QLibraryInfo>
#include <qjstest/test262runner.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_EcmaScriptTests : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_EcmaScriptTests()
        : QQmlDataTest(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::DoNotFailOnWarnings, "test262")
    {
        if (qgetenv("QTEST_FUNCTION_TIMEOUT").isEmpty())
            qputenv("QTEST_FUNCTION_TIMEOUT", "900000");
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
    const QByteArrayView noisy("qt.qml.compiler");
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
    /* Suppress lcQmlCompiler's "qt.qml.compiler" warnings; we aren't in a
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
#if defined(Q_PROCESSOR_X86_64)
    Test262Runner runner(QString(), dataDirectory(), directory() + QStringLiteral("/TestExpectations"));
    runner.setFlags(Test262Runner::ForceBytecode
                    | Test262Runner::WithTestExpectations
                    | Test262Runner::Parallel
                    | Test262Runner::Verbose);
    bool result = runner.run();
    QVERIFY(result);
#endif
}

void tst_EcmaScriptTests::runJitted()
{
#if defined(Q_PROCESSOR_X86_64)
    Test262Runner runner(QString(), dataDirectory(), directory() + QStringLiteral("/TestExpectations"));
    runner.setFlags(Test262Runner::ForceJIT
                    | Test262Runner::WithTestExpectations
                    | Test262Runner::Parallel
                    | Test262Runner::Verbose);
    bool result = runner.run();
    QVERIFY(result);
#endif
}

QTEST_GUILESS_MAIN(tst_EcmaScriptTests)

#include "tst_ecmascripttests.moc"

