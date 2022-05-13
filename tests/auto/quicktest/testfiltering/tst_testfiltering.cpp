// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QProcess>
#include <QtTest>

class tst_TestFiltering : public QObject
{
    Q_OBJECT
private slots:
    void noFilters();
    void oneMatchingFilter();
    void filterThatDoesntMatch();
    void twoFilters();
    void twoFiltersWithOneMatch();
    void manyFilters();
    void filterTestWithDefaultDataTags();
    void filterTestWithDataTags();
    void filterTestByDataTag();
    void filterInvalidDataTag();
};


const QString testExe =
#if defined(Q_OS_WIN)
    QFINDTESTDATA("quicktestmain/quicktestmain.exe");
#else
    QFINDTESTDATA("quicktestmain/quicktestmain");
#endif

void tst_TestFiltering::noFilters()
{
    QProcess process;
    process.start(testExe);

    QVERIFY(process.waitForFinished());

    const QString output = process.readAll();
    QVERIFY(output.contains(QLatin1String("Totals: 17 passed")));
    QVERIFY(output.contains(QLatin1String(", 3 skipped")));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

void tst_TestFiltering::oneMatchingFilter()
{
    QProcess process;
    process.start(testExe, {QLatin1String("First::test_bar")});

    QVERIFY(process.waitForFinished());

    const QString output = process.readAll();
    QVERIFY(output.contains(QLatin1String("Totals: 3 passed")));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

void tst_TestFiltering::filterThatDoesntMatch()
{
    QProcess process;
    process.start(testExe, {QLatin1String("First::test_nonexisting")});

    QVERIFY(process.waitForFinished());

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 1);
}

void tst_TestFiltering::twoFilters()
{
    QProcess process;
    process.start(testExe,
                  {QLatin1String("Second::test_dupfoo"), QLatin1String("Second::test_dupbaz")});

    QVERIFY(process.waitForFinished());

    const QString output = process.readAll();
    QVERIFY(output.contains(QLatin1String("Totals: 4 passed")));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

void tst_TestFiltering::twoFiltersWithOneMatch()
{
    QProcess process;
    process.start(testExe,
                  {QLatin1String("First::test_foo"), QLatin1String("Second::test_nonexisting")});

    QVERIFY(process.waitForFinished());

    const QString output = process.readAll();
    QVERIFY(output.contains(QLatin1String("Totals: 3 passed")));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 1);
}

void tst_TestFiltering::manyFilters()
{
    QProcess process;
    process.start(testExe,
                  {QLatin1String("First::test_foo"),
                   QLatin1String("First::test_baz"),
                   QLatin1String("Second::test_dupfoo"),
                   QLatin1String("Second::test_dupbaz")});

    QVERIFY(process.waitForFinished());

    const QString output = process.readAll();
    QVERIFY(output.contains(QLatin1String("Totals: 8 passed")));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

void tst_TestFiltering::filterTestWithDefaultDataTags()
{
    QProcess process;
    process.start(testExe, { QLatin1String("Third::test_default_tags"), });

    QVERIFY(process.waitForFinished());

    const QString output = process.readAll();
    QVERIFY(output.contains(QLatin1String("Totals: 5 passed")));
    QVERIFY(output.contains(QLatin1String(" 2 skipped")));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

void tst_TestFiltering::filterTestWithDataTags()
{
    QProcess process;
    process.start(testExe, { QLatin1String("Third::test_tags"), });

    QVERIFY(process.waitForFinished());

    const QString output = process.readAll();
    QVERIFY(output.contains(QLatin1String("Totals: 4 passed")));
    QVERIFY(output.contains(QLatin1String(" 1 skipped")));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

void tst_TestFiltering::filterTestByDataTag()
{
    QProcess process;
    process.start(testExe, { QLatin1String("Third::test_default_tags:init_2"),
                             QLatin1String("Third::test_default_tags:skip_3"),
                             QLatin1String("Third::test_tags:baz"),
                             QLatin1String("Third::test_tags:bar"), });

    QVERIFY(process.waitForFinished());

    const QString output = process.readAll();
    QVERIFY(output.contains(QLatin1String("Totals: 4 passed")));
    QVERIFY(output.contains(QLatin1String(" 2 skipped")));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
}

void tst_TestFiltering::filterInvalidDataTag()
{
    QProcess process;
    process.start(testExe, { QLatin1String("Third::test_tags:invalid_tag") });

    QVERIFY(process.waitForFinished());

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 1);
}

QTEST_MAIN(tst_TestFiltering);

#include "tst_testfiltering.moc"
