/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include <QtQml>
#include <QtTest>

//#define QT_QUICK_CONTROLS_V1

class tst_Creation : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testControls();
    void testControls_data();

    void testCalendar();
    void testCalendar_data();

private:
    QQmlEngine engine;
};

void tst_Creation::initTestCase()
{
    engine.clearComponentCache();
}

void tst_Creation::testControls()
{
    QFETCH(QByteArray, control);

    QQmlComponent component(&engine);
#ifdef QT_QUICK_CONTROLS_V1
    component.setData("import QtQuick.Controls 1.3;" + control + "{}", QUrl());
#else
    component.setData("import QtQuick.Controls 2.0;" + control + "{}", QUrl());
#endif

    QObjectList objects;
    QBENCHMARK {
        QObject *object = component.create();
        if (!object)
            qFatal("%s", qPrintable(component.errorString()));
        objects += object;
    }
    qDeleteAll(objects);
    engine.clearComponentCache();
}

void tst_Creation::testControls_data()
{
    QTest::addColumn<QByteArray>("control");

    QTest::newRow("ApplicationWindow") << QByteArray("ApplicationWindow");
    QTest::newRow("BusyIndicator") << QByteArray("BusyIndicator");
    QTest::newRow("Button") << QByteArray("Button");
    QTest::newRow("CheckBox") << QByteArray("CheckBox");
#ifndef QT_QUICK_CONTROLS_V1
    QTest::newRow("Frame") << QByteArray("Frame");
#endif
    QTest::newRow("GroupBox") << QByteArray("GroupBox");
    QTest::newRow("Label") << QByteArray("Label");
    QTest::newRow("ProgressBar") << QByteArray("ProgressBar");
    QTest::newRow("RadioButton") << QByteArray("RadioButton");
#ifdef QT_QUICK_CONTROLS_V1
    QTest::newRow("ScrollView") << QByteArray("ScrollView");
#else
    QTest::newRow("ScrollBar") << QByteArray("ScrollBar");
    QTest::newRow("ScrollIndicator") << QByteArray("ScrollIndicator");
#endif
    QTest::newRow("Slider") << QByteArray("Slider");
    QTest::newRow("SpinBox") << QByteArray("SpinBox");
    QTest::newRow("Switch") << QByteArray("Switch");
#ifndef QT_QUICK_CONTROLS_V1
    QTest::newRow("TabBar") << QByteArray("TabBar");
    QTest::newRow("TabButton") << QByteArray("TabButton");
#endif
    QTest::newRow("TabView") << QByteArray("TabView");
    QTest::newRow("TextArea") << QByteArray("TextArea");
    QTest::newRow("TextField") << QByteArray("TextField");
    QTest::newRow("ToolBar") << QByteArray("ToolBar");
    QTest::newRow("ToolButton") << QByteArray("ToolButton");
}

void tst_Creation::testCalendar()
{
    QFETCH(QByteArray, control);

    QQmlComponent component(&engine);
#ifdef QT_QUICK_CONTROLS_V1
    component.setData("import QtQuick.Controls 1.3;" + control + "{}", QUrl());
#else
    component.setData("import QtQuick.Calendar 2.0;" + control + "{}", QUrl());
#endif

    QObjectList objects;
    QBENCHMARK {
        QObject *object = component.create();
        if (!object)
            qFatal("%s", qPrintable(component.errorString()));
        objects += object;
    }
    qDeleteAll(objects);
    engine.clearComponentCache();
}

void tst_Creation::testCalendar_data()
{
    QTest::addColumn<QByteArray>("control");

#ifdef QT_QUICK_CONTROLS_V1
    QTest::newRow("Calendar") << QByteArray("Calendar");
#else
    QTest::newRow("CalendarModel") << QByteArray("CalendarModel");
    QTest::newRow("CalendarView") << QByteArray("CalendarView");
    QTest::newRow("DayOfWeekRow") << QByteArray("DayOfWeekRow");
    QTest::newRow("WeekNumberColumn") << QByteArray("WeekNumberColumn");
#endif
}

QTEST_MAIN(tst_Creation)

#include "tst_creation.moc"
