// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtTest/QtTest>
#include <QProcess>
#include <QLibraryInfo>
#include <qjstest/test262runner.h>

class tst_EcmaScriptTests : public QObject
{
    Q_OBJECT

private slots:
    void runInterpreted();
    void runJitted();
};

void tst_EcmaScriptTests::runInterpreted()
{
#if defined(Q_PROCESSOR_X86_64)
    QDir::setCurrent(QLatin1String(SRCDIR));
    Test262Runner runner(QString(), "test262");
    runner.setFlags(Test262Runner::ForceBytecode|Test262Runner::WithTestExpectations|Test262Runner::Parallel|Test262Runner::Verbose);
    bool result = runner.run();
    QVERIFY(result);
#endif
}

void tst_EcmaScriptTests::runJitted()
{
#if defined(Q_PROCESSOR_X86_64)
    QDir::setCurrent(QLatin1String(SRCDIR));
    Test262Runner runner(QString(), "test262");
    runner.setFlags(Test262Runner::ForceJIT|Test262Runner::WithTestExpectations|Test262Runner::Parallel|Test262Runner::Verbose);
    bool result = runner.run();
    QVERIFY(result);
#endif
}

QTEST_GUILESS_MAIN(tst_EcmaScriptTests)

#include "tst_ecmascripttests.moc"

