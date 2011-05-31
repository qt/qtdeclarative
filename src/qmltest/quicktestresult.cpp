/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "quicktestresult_p.h"
#include <QtTest/qtestcase.h>
#include <QtTest/qtestsystem.h>
#include <QtTest/private/qtestresult_p.h>
#include <QtTest/private/qtesttable_p.h>
#include <QtTest/private/qtestlog_p.h>
#include "qtestoptions_p.h"
#include <QtTest/qbenchmark.h>
#include <QtTest/private/qbenchmark_p.h>
#include <QtCore/qset.h>
#include <QtCore/qmap.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

static const char *globalProgramName = 0;
static bool loggingStarted = false;
static QBenchmarkGlobalData globalBenchmarkData;

class QuickTestResultPrivate
{
public:
    QuickTestResultPrivate()
        : table(0)
        , benchmarkIter(0)
        , benchmarkData(0)
        , iterCount(0)
    {
    }
    ~QuickTestResultPrivate()
    {
        delete table;
        delete benchmarkIter;
        delete benchmarkData;
    }

    QByteArray intern(const QString &str);
    void updateTestObjectName();

    QString testCaseName;
    QString functionName;
    QSet<QByteArray> internedStrings;
    QTestTable *table;
    QTest::QBenchmarkIterationController *benchmarkIter;
    QBenchmarkTestMethodData *benchmarkData;
    int iterCount;
    QList<QBenchmarkResult> results;
};

QByteArray QuickTestResultPrivate::intern(const QString &str)
{
    QByteArray bstr = str.toUtf8();
    return *(internedStrings.insert(bstr));
}

void QuickTestResultPrivate::updateTestObjectName()
{
    // In plain logging mode we use the TestCase name as the
    // class name so that multiple TestCase elements will report
    // results with "testCase::function".  In XML logging mode,
    // we use the program name as the class name and report test
    // functions as "testCase__function".
    if (QTestLog::logMode() == QTestLog::Plain) {
        if (testCaseName.isEmpty()) {
            QTestResult::setCurrentTestObject(globalProgramName);
        } else if (QTestLog::logMode() == QTestLog::Plain) {
            QTestResult::setCurrentTestObject
                (intern(testCaseName).constData());
        }
    } else {
        QTestResult::setCurrentTestObject(globalProgramName);
    }
}

QuickTestResult::QuickTestResult(QObject *parent)
    : QObject(parent), d_ptr(new QuickTestResultPrivate)
{
    if (!QBenchmarkGlobalData::current)
        QBenchmarkGlobalData::current = &globalBenchmarkData;
}

QuickTestResult::~QuickTestResult()
{
}

/*!
    \qmlproperty string TestResult::testCaseName

    This property defines the name of current TestCase element
    that is running test cases.

    \sa functionName
*/
QString QuickTestResult::testCaseName() const
{
    Q_D(const QuickTestResult);
    return d->testCaseName;
}

void QuickTestResult::setTestCaseName(const QString &name)
{
    Q_D(QuickTestResult);
    d->testCaseName = name;
    d->updateTestObjectName();
    emit testCaseNameChanged();
}

/*!
    \qmlproperty string TestResult::functionName

    This property defines the name of current test function
    within a TestCase element that is running.  If this string is
    empty, then no function is currently running.

    \sa testCaseName
*/
QString QuickTestResult::functionName() const
{
    Q_D(const QuickTestResult);
    return d->functionName;
}

void QuickTestResult::setFunctionName(const QString &name)
{
    Q_D(QuickTestResult);
    if (!name.isEmpty()) {
        // In plain logging mode, we use the function name directly.
        // In XML logging mode, we use "testCase__functionName" as the
        // program name is acting as the class name.
        if (QTestLog::logMode() == QTestLog::Plain ||
                d->testCaseName.isEmpty()) {
            QTestResult::setCurrentTestFunction
                (d->intern(name).constData());
        } else {
            QString fullName = d->testCaseName + QLatin1String("__") + name;
            QTestResult::setCurrentTestFunction
                (d->intern(fullName).constData());
        }
    } else {
        QTestResult::setCurrentTestFunction(0);
    }
    d->functionName = name;
    emit functionNameChanged();
}

QuickTestResult::FunctionType QuickTestResult::functionType() const
{
    return FunctionType(QTestResult::currentTestLocation());
}

void QuickTestResult::setFunctionType(FunctionType type)
{
    QTestResult::setCurrentTestLocation(QTestResult::TestLocation(type));
    emit functionTypeChanged();
}

/*!
    \qmlproperty string TestResult::dataTag

    This property defines the tag for the current row in a
    data-driven test, or an empty string if not a data-driven test.
*/
QString QuickTestResult::dataTag() const
{
    const char *tag = QTestResult::currentDataTag();
    if (tag)
        return QString::fromUtf8(tag);
    else
        return QString();
}

void QuickTestResult::setDataTag(const QString &tag)
{
    if (!tag.isEmpty()) {
        QTestData *data = &(QTest::newRow(tag.toUtf8().constData()));
        QTestResult::setCurrentTestData(data);
        emit dataTagChanged();
    } else {
        QTestResult::setCurrentTestData(0);
    }
}

/*!
    \qmlproperty bool TestResult::failed

    This property returns true if the current test function has
    failed; false otherwise.  The fail state is reset when
    functionName is changed or finishTestFunction() is called.

    \sa skipped, dataFailed
*/
bool QuickTestResult::isFailed() const
{
    return QTestResult::testFailed();
}

/*!
    \qmlproperty bool TestResult::dataFailed

    This property returns true if the current data function has
    failed; false otherwise.  The fail state is reset when
    functionName is changed or finishTestFunction() is called.

    \sa failed
*/
bool QuickTestResult::isDataFailed() const
{
    return QTestResult::currentTestFailed();
}

/*!
    \qmlproperty bool TestResult::skipped

    This property returns true if the current test function was
    marked as skipped; false otherwise.

    \sa failed
*/
bool QuickTestResult::isSkipped() const
{
    return QTestResult::skipCurrentTest();
}

void QuickTestResult::setSkipped(bool skip)
{
    QTestResult::setSkipCurrentTest(skip);
    emit skippedChanged();
}

/*!
    \qmlproperty int TestResult::passCount

    This property returns the number of tests that have passed.

    \sa failCount, skipCount
*/
int QuickTestResult::passCount() const
{
    return QTestResult::passCount();
}

/*!
    \qmlproperty int TestResult::failCount

    This property returns the number of tests that have failed.

    \sa passCount, skipCount
*/
int QuickTestResult::failCount() const
{
    return QTestResult::failCount();
}

/*!
    \qmlproperty int TestResult::skipCount

    This property returns the number of tests that have been skipped.

    \sa passCount, failCount
*/
int QuickTestResult::skipCount() const
{
    return QTestResult::skipCount();
}

/*!
    \qmlproperty list<string> TestResult::functionsToRun

    This property returns the list of function names to be run.
*/
QStringList QuickTestResult::functionsToRun() const
{
    return QTest::testFunctions;
}

/*!
    \qmlmethod TestResult::reset()

    Resets all pass/fail/skip counters and prepare for testing.
*/
void QuickTestResult::reset()
{
    if (!globalProgramName)     // Only if run via qmlviewer.
        QTestResult::reset();
}

/*!
    \qmlmethod TestResult::startLogging()

    Starts logging to the test output stream and writes the
    test header.

    \sa stopLogging()
*/
void QuickTestResult::startLogging()
{
    // The program name is used for logging headers and footers if it
    // is set.  Otherwise the test case name is used.
    Q_D(QuickTestResult);
    if (loggingStarted)
        return;
    const char *saved = QTestResult::currentTestObjectName();
    if (globalProgramName) {
        QTestResult::setCurrentTestObject(globalProgramName);
    } else {
        QTestResult::setCurrentTestObject
            (d->intern(d->testCaseName).constData());
    }
    QTestLog::startLogging();
    QTestResult::setCurrentTestObject(saved);
    loggingStarted = true;
}

/*!
    \qmlmethod TestResult::stopLogging()

    Writes the test footer to the test output stream and then stops logging.

    \sa startLogging()
*/
void QuickTestResult::stopLogging()
{
    Q_D(QuickTestResult);
    if (globalProgramName)
        return;     // Logging will be stopped by setProgramName(0).
    const char *saved = QTestResult::currentTestObjectName();
    QTestResult::setCurrentTestObject(d->intern(d->testCaseName).constData());
    QTestLog::stopLogging();
    QTestResult::setCurrentTestObject(saved);
}

void QuickTestResult::initTestTable()
{
    Q_D(QuickTestResult);
    delete d->table;
    d->table = new QTestTable;
}

void QuickTestResult::clearTestTable()
{
    Q_D(QuickTestResult);
    delete d->table;
    d->table = 0;
}

void QuickTestResult::finishTestFunction()
{
    QTestResult::finishedCurrentTestFunction();
}

static QString qtest_fixFile(const QString &file)
{
    if (file.startsWith(QLatin1String("file://")))
        return file.mid(7);
    else
        return file;
}

void QuickTestResult::fail
    (const QString &message, const QString &file, int line)
{
    QTestResult::addFailure(message.toLatin1().constData(),
                            qtest_fixFile(file).toLatin1().constData(), line);
}

bool QuickTestResult::verify
    (bool success, const QString &message, const QString &file, int line)
{
    if (!success && message.isEmpty()) {
        return QTestResult::verify
            (success, "verify()", "",
             qtest_fixFile(file).toLatin1().constData(), line);
    } else {
        return QTestResult::verify
            (success, message.toLatin1().constData(), "",
             qtest_fixFile(file).toLatin1().constData(), line);
    }
}

bool QuickTestResult::compare
    (bool success, const QString &message,
     const QString &val1, const QString &val2,
     const QString &file, int line)
{
    if (success) {
        return QTestResult::compare
            (success, message.toLocal8Bit().constData(),
             qtest_fixFile(file).toLatin1().constData(), line);
    } else {
        return QTestResult::compare
            (success, message.toLocal8Bit().constData(),
             QTest::toString(val1.toLatin1().constData()),
             QTest::toString(val2.toLatin1().constData()),
             "", "",
             qtest_fixFile(file).toLatin1().constData(), line);
    }
}

void QuickTestResult::skipSingle
    (const QString &message, const QString &file, int line)
{
    QTestResult::addSkip(message.toLatin1().constData(), QTest::SkipSingle,
                         qtest_fixFile(file).toLatin1().constData(), line);
}

void QuickTestResult::skipAll
    (const QString &message, const QString &file, int line)
{
    QTestResult::addSkip(message.toLatin1().constData(), QTest::SkipAll,
                         qtest_fixFile(file).toLatin1().constData(), line);
    QTestResult::setSkipCurrentTest(true);
}

bool QuickTestResult::expectFail
    (const QString &tag, const QString &comment, const QString &file, int line)
{
    return QTestResult::expectFail
        (tag.toLatin1().constData(),
         QTest::toString(comment.toLatin1().constData()),
         QTest::Abort, qtest_fixFile(file).toLatin1().constData(), line);
}

bool QuickTestResult::expectFailContinue
    (const QString &tag, const QString &comment, const QString &file, int line)
{
    return QTestResult::expectFail
        (tag.toLatin1().constData(),
         QTest::toString(comment.toLatin1().constData()),
         QTest::Continue, qtest_fixFile(file).toLatin1().constData(), line);
}

void QuickTestResult::warn(const QString &message)
{
    QTestLog::warn(message.toLatin1().constData());
}

void QuickTestResult::ignoreWarning(const QString &message)
{
    QTestResult::ignoreMessage(QtWarningMsg, message.toLatin1().constData());
}

void QuickTestResult::wait(int ms)
{
    QTest::qWait(ms);
}

void QuickTestResult::sleep(int ms)
{
    QTest::qSleep(ms);
}

void QuickTestResult::startMeasurement()
{
    Q_D(QuickTestResult);
    delete d->benchmarkData;
    d->benchmarkData = new QBenchmarkTestMethodData();
    QBenchmarkTestMethodData::current = d->benchmarkData;
    d->iterCount = (QBenchmarkGlobalData::current->measurer->needsWarmupIteration()) ? -1 : 0;
    d->results.clear();
}

void QuickTestResult::beginDataRun()
{
    QBenchmarkTestMethodData::current->beginDataRun();
}

void QuickTestResult::endDataRun()
{
    Q_D(QuickTestResult);
    QBenchmarkTestMethodData::current->endDataRun();
    if (d->iterCount > -1)  // iteration -1 is the warmup iteration.
        d->results.append(QBenchmarkTestMethodData::current->result);

    if (QBenchmarkGlobalData::current->verboseOutput) {
        if (d->iterCount == -1) {
            qDebug() << "warmup stage result      :" << QBenchmarkTestMethodData::current->result.value;
        } else {
            qDebug() << "accumulation stage result:" << QBenchmarkTestMethodData::current->result.value;
        }
    }
}

bool QuickTestResult::measurementAccepted()
{
    return QBenchmarkTestMethodData::current->resultsAccepted();
}

static QBenchmarkResult qMedian(const QList<QBenchmarkResult> &container)
{
    const int count = container.count();
    if (count == 0)
        return QBenchmarkResult();

    if (count == 1)
        return container.at(0);

    QList<QBenchmarkResult> containerCopy = container;
    qSort(containerCopy);

    const int middle = count / 2;

    // ### handle even-sized containers here by doing an aritmetic mean of the two middle items.
    return containerCopy.at(middle);
}

bool QuickTestResult::needsMoreMeasurements()
{
    Q_D(QuickTestResult);
    ++(d->iterCount);
    if (d->iterCount < QBenchmarkGlobalData::current->adjustMedianIterationCount())
        return true;
    if (QBenchmarkTestMethodData::current->resultsAccepted())
        QTestLog::addBenchmarkResult(qMedian(d->results));
    return false;
}

void QuickTestResult::startBenchmark(RunMode runMode, const QString &tag)
{
    QBenchmarkTestMethodData::current->result = QBenchmarkResult();
    QBenchmarkTestMethodData::current->resultAccepted = false;
    QBenchmarkGlobalData::current->context.tag = tag;
    QBenchmarkGlobalData::current->context.slotName = functionName();

    Q_D(QuickTestResult);
    delete d->benchmarkIter;
    d->benchmarkIter = new QTest::QBenchmarkIterationController
        (QTest::QBenchmarkIterationController::RunMode(runMode));
}

bool QuickTestResult::isBenchmarkDone() const
{
    Q_D(const QuickTestResult);
    if (d->benchmarkIter)
        return d->benchmarkIter->isDone();
    else
        return true;
}

void QuickTestResult::nextBenchmark()
{
    Q_D(QuickTestResult);
    if (d->benchmarkIter)
        d->benchmarkIter->next();
}

void QuickTestResult::stopBenchmark()
{
    Q_D(QuickTestResult);
    delete d->benchmarkIter;
    d->benchmarkIter = 0;
}

namespace QTest {
    void qtest_qParseArgs(int argc, char *argv[], bool qml);
};

void QuickTestResult::parseArgs(int argc, char *argv[])
{
    if (!QBenchmarkGlobalData::current)
        QBenchmarkGlobalData::current = &globalBenchmarkData;
    QTest::qtest_qParseArgs(argc, argv, true);
}

void QuickTestResult::setProgramName(const char *name)
{
    if (name) {
        QTestResult::reset();
    } else if (!name && loggingStarted) {
        QTestResult::setCurrentTestObject(globalProgramName);
        QTestLog::stopLogging();
        QTestResult::setCurrentTestObject(0);
    }
    globalProgramName = name;
}

int QuickTestResult::exitCode()
{
#if defined(QTEST_NOEXITCODE)
    return 0;
#else
    // make sure our exit code is never going above 127
    // since that could wrap and indicate 0 test fails
    return qMin(QTestResult::failCount(), 127);
#endif
}

QT_END_NAMESPACE
