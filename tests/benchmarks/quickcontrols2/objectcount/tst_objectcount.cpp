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

#include <QtTest>
#include <QtQuick>
#include <QtCore/private/qhooks_p.h>
#include <iostream>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>

using namespace QQuickVisualTestUtils;
using namespace QQuickControlsTestUtils;

static int qt_verbose = qEnvironmentVariableIntValue("VERBOSE") != 0;

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

    void qobjects();
    void qobjects_data();

    void qquickitems();
    void qquickitems_data();

private:
    QQmlEngine engine;
};

void tst_ObjectCount::init()
{
    qtHookData[QHooks::AddQObject] = reinterpret_cast<quintptr>(&qt_addQObject);
    qtHookData[QHooks::RemoveQObject] = reinterpret_cast<quintptr>(&qt_removeQObject);

    // warmup
    QQmlComponent component(&engine);
    component.setData("import QtQuick; import QtQuick.Controls; Item { Button {} }", QUrl());
    delete component.create();
}

void tst_ObjectCount::cleanup()
{
    qtHookData[QHooks::AddQObject] = 0;
    qtHookData[QHooks::RemoveQObject] = 0;
}

static void initTestRows(QQmlEngine *engine)
{
    addTestRowForEachControl(engine, QQC2_IMPORT_PATH, "basic", "QtQuick/Controls/Basic");
    addTestRowForEachControl(engine, QQC2_IMPORT_PATH, "fusion", "QtQuick/Controls/Fusion",
        QStringList() << "ButtonPanel" << "CheckIndicator" << "RadioIndicator" << "SliderGroove" << "SliderHandle" << "SwitchIndicator");
    addTestRowForEachControl(engine, QQC2_IMPORT_PATH, "imagine", "QtQuick/Controls/Imagine");
    addTestRowForEachControl(engine, QQC2_IMPORT_PATH, "material", "QtQuick/Controls/Material",
        QStringList() << "Ripple" << "SliderHandle" << "CheckIndicator" << "RadioIndicator"
            << "SwitchIndicator" << "BoxShadow" << "ElevationEffect" << "CursorDelegate");
    addTestRowForEachControl(engine, QQC2_IMPORT_PATH, "universal", "QtQuick/Controls/Universal",
        QStringList() << "CheckIndicator" << "RadioIndicator" << "SwitchIndicator");
}

template <typename T>
static void doBenchmark(QQmlEngine *engine, const QUrl &url)
{
    QQmlComponent component(engine);

    qt_qobjects->clear();

    component.loadUrl(url);
    QScopedPointer<QObject> object(component.create());
    QVERIFY2(object.data(), qPrintable(component.errorString()));

    QObjectList objects;
    for (QObject *object : qAsConst(*qt_qobjects())) {
        if (qobject_cast<T *>(object))
            objects += object;
    }

    if (qt_verbose) {
        for (QObject *object : objects)
            qInfo() << "\t" << object;
    }

    QTest::setBenchmarkResult(objects.count(), QTest::Events);
}

void tst_ObjectCount::qobjects()
{
    QFETCH(QUrl, url);
    doBenchmark<QObject>(&engine, url);
}

void tst_ObjectCount::qobjects_data()
{
    QTest::addColumn<QUrl>("url");
    initTestRows(&engine);
}

void tst_ObjectCount::qquickitems()
{
    QFETCH(QUrl, url);
    doBenchmark<QQuickItem>(&engine, url);
}

void tst_ObjectCount::qquickitems_data()
{
    QTest::addColumn<QUrl>("url");
    initTestRows(&engine);
}

QTEST_MAIN(tst_ObjectCount)

#include "tst_objectcount.moc"
