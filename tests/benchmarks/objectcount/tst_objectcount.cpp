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

#include <QtTest>
#include <QtQuick>
#include <QtCore/private/qhooks_p.h>

static int qt_verbose = qgetenv("VERBOSE").toInt() != 0;

Q_GLOBAL_STATIC(QObjectList, qt_qobjects)

extern "C" Q_DECL_EXPORT void qt_addQObject(QObject *object)
{
    qt_qobjects->append(object);
}

extern "C" Q_DECL_EXPORT void qt_removeQObject(QObject *object)
{
    qt_qobjects->removeAll(object);
}

class tst_ObjectCount : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testCount();
    void testCount_data();

private:
    QQmlEngine engine;
};

void tst_ObjectCount::init()
{
    qtHookData[QHooks::AddQObject] = reinterpret_cast<quintptr>(&qt_addQObject);
    qtHookData[QHooks::RemoveQObject] = reinterpret_cast<quintptr>(&qt_removeQObject);

    // warmup
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0; import QtQuick.Controls 1.3; import QtQuick.Controls 2.0; Image { Accessible.role: Accessible.Button; source: 'qrc:/qt-logo.png' }", QUrl());
    delete component.create();
}

void tst_ObjectCount::cleanup()
{
    qtHookData[QHooks::AddQObject] = 0;
    qtHookData[QHooks::RemoveQObject] = 0;
}

static void printItems(const QString &prefix, const QList<QQuickItem *> &items)
{
    qInfo() << prefix << "QQuickItems:" << items.count() << "(total of QObjects:" << qt_qobjects->count() << ")";
    if (qt_verbose) {
        foreach (QObject *object, *qt_qobjects)
            qInfo() << "\t" << object;
    }
}

void tst_ObjectCount::testCount()
{
    QFETCH(QByteArray, v1);
    QFETCH(QByteArray, v2);

    qt_qobjects->clear();

    if (!v1.isEmpty()) {
        QQmlComponent component(&engine);
        component.setData(v1, QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object.data(), qPrintable(component.errorString()));

        QList<QQuickItem *> items;
        foreach (QObject *object, *qt_qobjects()) {
            QQuickItem *item = qobject_cast<QQuickItem *>(object);
            if (item)
                items += item;
        }
        printItems("V1", items);
    }

    qt_qobjects->clear();

    if (!v2.isEmpty()) {
        QQmlComponent component(&engine);
        component.setData(v2, QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object.data(), qPrintable(component.errorString()));

        QList<QQuickItem *> items;
        foreach (QObject *object, *qt_qobjects()) {
            QQuickItem *item = qobject_cast<QQuickItem *>(object);
            if (item)
                items += item;
        }
        printItems("V2", items);
    }
}

void tst_ObjectCount::testCount_data()
{
    QTest::addColumn<QByteArray>("v1");
    QTest::addColumn<QByteArray>("v2");

    QTest::newRow("ApplicationWindow")
            << QByteArray("import QtQuick.Controls 1.3; ApplicationWindow { }")
            << QByteArray("import QtQuick.Controls 2.0; ApplicationWindow { }");

    QTest::newRow("BusyIndicator")
            << QByteArray("import QtQuick.Controls 1.3; BusyIndicator { }")
            << QByteArray("import QtQuick.Controls 2.0; BusyIndicator { }");

    QTest::newRow("Button")
            << QByteArray("import QtQuick.Controls 1.3; Button { }")
            << QByteArray("import QtQuick.Controls 2.0; Button { }");

    QTest::newRow("CheckBox")
            << QByteArray("import QtQuick.Controls 1.3; CheckBox { }")
            << QByteArray("import QtQuick.Controls 2.0; CheckBox { }");

    QTest::newRow("Frame")
            << QByteArray()
            << QByteArray("import QtQuick.Controls 2.0; Frame { }");

    QTest::newRow("GroupBox")
            << QByteArray("import QtQuick.Controls 1.3; GroupBox { }")
            << QByteArray("import QtQuick.Controls 2.0; GroupBox { }");

    QTest::newRow("Label")
            << QByteArray("import QtQuick.Controls 1.3; Label { }")
            << QByteArray("import QtQuick.Controls 2.0; Label { }");

    QTest::newRow("PageIndicator")
            << QByteArray()
            << QByteArray("import QtQuick.Controls 2.0; PageIndicator { }");

    QTest::newRow("ProgressBar")
            << QByteArray("import QtQuick.Controls 1.3; ProgressBar { }")
            << QByteArray("import QtQuick.Controls 2.0; ProgressBar { }");

    QTest::newRow("RadioButton")
            << QByteArray("import QtQuick.Controls 1.3; RadioButton { }")
            << QByteArray("import QtQuick.Controls 2.0; RadioButton { }");

    QTest::newRow("ScrollView")
            << QByteArray("import QtQuick.Controls 1.3; ScrollView { }")
            << QByteArray();

    QTest::newRow("ScrollIndicator")
            << QByteArray()
            << QByteArray("import QtQuick.Controls 2.0; ScrollIndicator { }");

    QTest::newRow("ScrollBar")
            << QByteArray()
            << QByteArray("import QtQuick.Controls 2.0; ScrollBar { }");

    QTest::newRow("Slider")
            << QByteArray("import QtQuick.Controls 1.3; Slider { }")
            << QByteArray("import QtQuick.Controls 2.0; Slider { }");

    QTest::newRow("StackView")
            << QByteArray("import QtQuick.Controls 1.3; StackView { }")
            << QByteArray("import QtQuick.Controls 2.0; StackView { }");

    QTest::newRow("Switch")
            << QByteArray("import QtQuick.Controls 1.3; Switch { }")
            << QByteArray("import QtQuick.Controls 2.0; Switch { }");

    QTest::newRow("TabBar")
            << QByteArray()
            << QByteArray("import QtQuick.Controls 2.0; TabBar { }");

    QTest::newRow("TabButton")
            << QByteArray()
            << QByteArray("import QtQuick.Controls 2.0; TabButton { }");

    QTest::newRow("TabView")
            << QByteArray("import QtQuick.Controls 1.3; TabView { }")
            << QByteArray("import QtQuick.Controls 2.0; TabView { }");

    QTest::newRow("TextArea")
            << QByteArray("import QtQuick.Controls 1.3; TextArea { }")
            << QByteArray("import QtQuick.Controls 2.0; TextArea { }");

    QTest::newRow("TextField")
            << QByteArray("import QtQuick.Controls 1.3; TextField { }")
            << QByteArray("import QtQuick.Controls 2.0; TextField { }");

    QTest::newRow("ToggleButton")
            << QByteArray()
            << QByteArray("import QtQuick.Controls 2.0; ToggleButton { }");

    QTest::newRow("ToolBar")
            << QByteArray("import QtQuick.Controls 1.3; ToolBar { }")
            << QByteArray("import QtQuick.Controls 2.0; ToolBar { }");

    QTest::newRow("ToolButton")
            << QByteArray("import QtQuick.Controls 1.3; ToolButton { }")
            << QByteArray("import QtQuick.Controls 2.0; ToolButton { }");
}

QTEST_MAIN(tst_ObjectCount)

#include "tst_objectcount.moc"
