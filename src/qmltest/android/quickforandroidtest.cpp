// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "quickforandroidtest_p.h"
#include "androidtestutils_p.h"

#include <QtQuickTest/private/quicktest_p.h>
#include <QtQuickTest/private/quicktestresult_p.h>
#include <QtTest/private/qtestcrashhandler_p.h>

QT_BEGIN_NAMESPACE

/*
 * For testing Qt Quick for Android (QQ4A) applications with QuickTest
 * Usage:
 *  QUICK_FOR_ANDROID_TEST_MAIN("test_name")
 *
 * This function does the same steps as the regular QtQuickTest macro, with one
 * exception: it does not parse tests from QML files. This is because in the
 * QQ4A context, the QML content is loaded by the hosting Android Activity,
 * via QtQuickView.java (QQuickView internally)
 *
 * So in order to execute tests, we need to just spin the event loop and wait
 * for the application to be manually quit by the tests. This is done via
 * QGuiApplication::exec(), which allows the QML to be loaded and the tests to
 * be run.
 *
 * Once there are no more testcases to run or the program has been aborted, we
 * continue the regular QtQuickTest routine, finishing up the QuickTestResult
 * logging and writing the Android exit code file, before returning out of the
 * function.
 */
int quick_for_android_test_main(int argc, char **argv, const char *name)
{
    // Allow exit() once we quit the Qt application.
    if (qEnvironmentVariableIsSet("QT_ANDROID_NO_EXIT_CALL"))
        ::unsetenv("QT_ANDROID_NO_EXIT_CALL");

    QScopedPointer<QCoreApplication> app;
    if (!QCoreApplication::instance())
        app.reset(new QGuiApplication(argc, argv));

    clearAndroidExitCode();

    QuickTestResult::setCurrentAppname(argv[0]);
    QuickTestResult::setProgramName(name);
    QuickTestResult::parseArgs(argc, argv);

    std::optional<QTest::CrashHandler::FatalSignalHandler> handler;
    QTest::CrashHandler::prepareStackTrace();
    if (!QTest::Internal::noCrashHandler)
        handler.emplace();

    qputenv("QT_QTESTLIB_RUNNING", "1");

    QTestRootObject::instance()->init();

    // To support "windowShown" property, set it to true once the event loop has started
    // (windows are always visible in the QQ4A context)
    QTimer::singleShot(0, [] { QTestRootObject::instance()->setWindowShown(true); });

    // Wait for TestCase.qml to call quit() once tests are done, till then just spin
    // TestSchedule.qml
    if (!QTestRootObject::instance()->hasQuit)
        app->exec();

    QuickTestResult::setProgramName(nullptr);
    app.reset();

    const int exitCode = QuickTestResult::exitCode();
    writeAndroidExitCode(exitCode);

    return exitCode;
}

QT_END_NAMESPACE
