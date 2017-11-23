/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QProcess>
#include <QLibraryInfo>

class tst_EcmaScriptTests : public QObject
{
    Q_OBJECT

    void runTests(bool interpret);

private slots:
    void runInterpreted();
    void runJitted();
};

void tst_EcmaScriptTests::runTests(bool interpret)
{
#if defined(Q_OS_LINUX) && defined(Q_PROCESSOR_X86_64)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (interpret)
        env.insert("QV4_FORCE_INTERPRETER", "1");
    else
        env.insert("QV4_JIT_CALL_THRESHOLD", "0");

    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.setWorkingDirectory(QLatin1String(SRCDIR));
    process.setProgram("python");
    process.setProcessEnvironment(env);
    process.setArguments(QStringList() << "test262.py" << "--command=" + QLibraryInfo::location(QLibraryInfo::BinariesPath) + "/qmljs" << "--parallel" << "--with-test-expectations");

    qDebug() << "Going to run" << process.program() << process.arguments() << "in" << process.workingDirectory();

    process.start();
    QVERIFY(process.waitForStarted());
    const int timeoutInMSecs = 20 * 60 * 1000;
    QVERIFY2(process.waitForFinished(timeoutInMSecs), "Tests did not terminate in time -- see output above for details");
    QVERIFY2(process.exitStatus() == QProcess::NormalExit, "Running the test harness failed -- see output above for details");
    QVERIFY2(process.exitCode() == 0, "Tests failed -- see output above for details");
#else
    QSKIP("Currently the ecmascript tests are only run on Linux/x86-64");
#endif
}

void tst_EcmaScriptTests::runInterpreted()
{
    runTests(true);
}

void tst_EcmaScriptTests::runJitted()
{
    runTests(false);
}

QTEST_GUILESS_MAIN(tst_EcmaScriptTests)

#include "tst_ecmascripttests.moc"

