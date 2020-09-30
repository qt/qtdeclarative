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

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuickControls2/qquickstyle.h>

class tst_revisions : public QObject
{
    Q_OBJECT

public:
    tst_revisions();

private slots:
    void revisions_data();
    void revisions();

    void window_data();
    void window();
};

tst_revisions::tst_revisions()
{
    QQuickStyle::setStyle("Basic");
}

void tst_revisions::revisions_data()
{
    QTest::addColumn<int>("revision");

    // Qt 5.7: 2.0, Qt 5.8: 2.1, Qt 5.9: 2.2...
    for (int i = 0; i <= 15; ++i)
        QTest::newRow(qPrintable(QString("2.%1").arg(i))) << i;
}

void tst_revisions::revisions()
{
    QFETCH(int, revision);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QString("import QtQuick; \
                               import QtQuick.Templates 2.%1 as T; \
                               import QtQuick.Controls 2.%1; \
                               import QtQuick.Controls.impl 2.%1; \
                               import QtQuick.Controls.Material 2.%1; \
                               import QtQuick.Controls.Material.impl 2.%1; \
                               import QtQuick.Controls.Universal 2.%1; \
                               import QtQuick.Controls.Universal.impl 2.%1; \
                               Control { }").arg(revision).toUtf8(), QUrl());

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));
}

void tst_revisions::window_data()
{
    QTest::addColumn<int>("revision");
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QString>("error");

    // Qt 5.7: 2.0, Qt 5.8: 2.1, Qt 5.9: 2.2
    // Note that we can no longer map the same Screen type to different versions
    // in QtQuick and QtQuick.Controls anymore.
    for (int i = 0; i <= 2; ++i)
        QTest::newRow(qPrintable(QString("screen:2.%1").arg(i))) << i << "screen: null" << QString(":1 \"ApplicationWindow.screen\" is not available in QtQuick.Templates 2.%1").arg(i);

    // Qt 5.10: 2.3...
    for (int i = 3; i <= 15; ++i)
        QTest::newRow(qPrintable(QString("screen:2.%1").arg(i))) << i << "screen: null" << "";
}

void tst_revisions::window()
{
    QFETCH(int, revision);
    QFETCH(QString, qml);
    QFETCH(QString, error);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QString("import QtQuick.Templates 2.%1; ApplicationWindow { %2 }").arg(revision).arg(qml).toUtf8(), QUrl());
    QScopedPointer<QObject> window(component.create());
    QCOMPARE(window.isNull(), !error.isEmpty());
}

QTEST_MAIN(tst_revisions)

#include "tst_revisions.moc"
