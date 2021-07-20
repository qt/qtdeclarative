/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTEST_QUICKCONTROLS_H
#define QTEST_QUICKCONTROLS_H

#include <QtTest/qtest.h>
#include <QtTest/private/qtestresult_p.h>
#include <QtGui/qguiapplication.h>
#include <QtQml/qqml.h>
#include <QtQuickControls2/qquickstyle.h>
#include <QtQuickControls2/private/qquickstyle_p.h>

static QStringList testStyles()
{
    // It's not enough to check if the name is empty, because since Qt 6
    // we set an appropriate style for the platform if no style was specified.
    // Also, we need the name check to come first, as isUsingDefaultStyle() does not do any resolving,
    // and so its return value wouldn't be correct otherwise.
    if (QQuickStyle::name().isEmpty() || QQuickStylePrivate::isUsingDefaultStyle())
        return QQuickStylePrivate::builtInStyles();
    return QStringList(QQuickStyle::name());
}

static int runTests(QObject *testObject, int argc, char *argv[])
{
    int res = 0;
    QTest::qInit(testObject, argc, argv);
    const QByteArray testObjectName = QTestResult::currentTestObjectName();
    // setCurrentTestObject() takes a C string, which means we must ensure
    // that the string we pass in lives long enough (i.e until the next call
    // to setCurrentTestObject()), so store the name outside of the loop.
    QByteArray testName;
    const QStringList styles = testStyles();
    for (const QString &style : styles) {
        qmlClearTypeRegistrations();
        QQuickStyle::setStyle(style);
        testName = testObjectName + "::" + style.toLocal8Bit();
        QTestResult::setCurrentTestObject(testName);
        res += QTest::qRun();
    }
    QTestResult::setCurrentTestObject(testObjectName);
    QTest::qCleanup();
    return res;
}

#define QTEST_QUICKCONTROLS_MAIN(TestCase) \
int main(int argc, char *argv[]) \
{ \
    qputenv("QML_NO_TOUCH_COMPRESSION", "1"); \
    QGuiApplication app(argc, argv); \
    TestCase tc; \
    QTEST_SET_MAIN_SOURCE_PATH \
    return runTests(&tc, argc, argv); \
}

#endif // QTEST_QUICKCONTROLS_H
