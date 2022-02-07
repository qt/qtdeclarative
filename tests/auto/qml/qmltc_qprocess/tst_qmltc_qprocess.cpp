/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QtTest/qtest.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qprocess.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qurl.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qfile.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qhash.h>

#include <functional>

class tst_qmltc_qprocess : public QQmlDataTest
{
    Q_OBJECT

    QString m_qmltcPath;
    QString m_tmpPath;
    QStringList m_resources;

    QString runQmltc(const QString &inputFile, std::function<void(QProcess &)> handleResult,
                     const QStringList &extraArgs = {});
    QString runQmltc(const QString &inputFile, bool shouldSucceed,
                     const QStringList &extraArgs = {});

    QString modifiedPath(const QString &path) { return path + u".orig"_qs; }

public:
    tst_qmltc_qprocess() : QQmlDataTest(QT_QMLTEST_DATADIR) { }

private slots:
    void initTestCase() override;
    void cleanupTestCase();

    void sanity();
    void noBuiltins();
    void noQtQml();
    void inlineComponent();
    void singleton();
};

#ifndef TST_QMLTC_QPROCESS_RESOURCES
#  error "This test expects TST_QMLTC_QPROCESS_RESOURCES to be defined through CMake."
#endif

void tst_qmltc_qprocess::initTestCase()
{
    QQmlDataTest::initTestCase();
    m_qmltcPath = QLibraryInfo::path(QLibraryInfo::BinariesPath) + u"/qmltc"_qs;
#ifdef Q_OS_WIN
    m_qmltcPath += u".exe"_qs;
#endif
    if (!QFileInfo(m_qmltcPath).exists()) {
        const QString message = u"qmltc executable not found (looked for %0)"_qs.arg(m_qmltcPath);
        QFAIL(qPrintable(message));
    }

    m_tmpPath = QDir::tempPath() + u"/tst_qmltc_qprocess_artifacts"_qs;
    QVERIFY(QDir(m_tmpPath).removeRecursively()); // in case it's already there
    QVERIFY(QDir().mkpath(m_tmpPath));

    m_resources = QStringLiteral(TST_QMLTC_QPROCESS_RESOURCES).split(u"_::_"_qs);
}

void tst_qmltc_qprocess::cleanupTestCase()
{
    QVERIFY(QDir(m_tmpPath).removeRecursively());
}

QString tst_qmltc_qprocess::runQmltc(const QString &inputFile,
                                     std::function<void(QProcess &)> handleResult,
                                     const QStringList &extraArgs)
{
    QStringList args;

    args << (QFileInfo(inputFile).isAbsolute() ? inputFile : testFile(inputFile));
    for (const QString &resource : m_resources)
        args << u"--resource"_qs << resource;
    args << u"--header"_qs << (m_tmpPath + u"/"_qs + QFileInfo(inputFile).baseName() + u".h"_qs);
    args << u"--impl"_qs << (m_tmpPath + u"/"_qs + QFileInfo(inputFile).baseName() + u".cpp"_qs);

    args << extraArgs;
    QString errors;

    QProcess process;
    process.start(m_qmltcPath, args);
    handleResult(process); // may fail the test
    errors = process.readAllStandardError();

    if (QTest::currentTestFailed()) {
        qDebug() << "Command:" << process.program() << args.join(u' ');
        qDebug() << "Exit status:" << process.exitStatus();
        qDebug() << "Exit code:" << process.exitCode();
        qDebug() << "stderr:" << errors;
        qDebug() << "stdout:" << process.readAllStandardOutput();
    }

    return errors;
}

QString tst_qmltc_qprocess::runQmltc(const QString &inputFile, bool shouldSucceed,
                                     const QStringList &extraArgs)
{
    return runQmltc(
            inputFile,
            [&](QProcess &process) {
                QVERIFY(process.waitForFinished());
                QCOMPARE(process.exitStatus(), QProcess::NormalExit);

                if (shouldSucceed)
                    QCOMPARE(process.exitCode(), 0);
                else
                    QVERIFY(process.exitCode() != 0);
            },
            extraArgs);
}

void tst_qmltc_qprocess::sanity()
{
    const auto output = runQmltc(u"dummy.qml"_qs, true);
    QVERIFY2(output.isEmpty(), qPrintable(output));
}

void tst_qmltc_qprocess::noBuiltins()
{
    const auto renameBack = [&](const QString &original) {
        const auto current = modifiedPath(original);
        QFile file(current);
        QVERIFY(file.exists());
        QVERIFY(file.rename(original));
    };

    for (QString builtin : { u"builtins.qmltypes"_qs, u"jsroot.qmltypes"_qs }) {
        const auto path = QLibraryInfo::path(QLibraryInfo::QmlImportsPath) + u"/"_qs + builtin;

        QScopeGuard scope(std::bind(renameBack, path));
        QFile file(path);
        QVERIFY(file.exists());
        QVERIFY(file.rename(modifiedPath(path)));

        // test that qmltc exits gracefully
        const auto errors = runQmltc(u"dummy.qml"_qs, false);
        QVERIFY(errors.contains(u"Failed to find the following builtins: %1"_qs.arg(builtin)));
    }
}

void tst_qmltc_qprocess::noQtQml()
{
    const auto renameBack = [&](const QString &original) {
        const auto current = modifiedPath(original);
        QVERIFY(QDir(current).exists());
        QVERIFY(QDir().rename(current, original));
    };

    const auto modulePath = QLibraryInfo::path(QLibraryInfo::QmlImportsPath) + u"/QtQml"_qs;
    QScopeGuard scope(std::bind(renameBack, modulePath));
    QVERIFY(QDir(modulePath).exists());
    QVERIFY(QDir().rename(modulePath, modifiedPath(modulePath)));

    // test that qmltc exits gracefully
    const auto errors = runQmltc(u"dummy.qml"_qs, false);
    QVERIFY(errors.contains(u"Failed to import QtQml. Are your include paths set up properly?"_qs));
}

void tst_qmltc_qprocess::inlineComponent()
{
    const auto errors = runQmltc(u"inlineComponent.qml"_qs, false);
    QEXPECT_FAIL("", "qmltc does not support inline components at the moment", Continue);
    QVERIFY(!errors.contains(u"Inline components are not supported"_qs));
}

void tst_qmltc_qprocess::singleton()
{
    const auto errors = runQmltc(u"SingletonThing.qml"_qs, false);
    QEXPECT_FAIL("", "qmltc does not support singletons at the moment", Continue);
    QVERIFY(!errors.contains(u"Singleton types are not supported"_qs));
}

QTEST_MAIN(tst_qmltc_qprocess)
#include "tst_qmltc_qprocess.moc"
