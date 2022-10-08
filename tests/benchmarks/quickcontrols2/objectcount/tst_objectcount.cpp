// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    // Calendar is excluded because it's a singleton and can't be created.
    // TreeViewDelegate is excluded since it's a delegate that can only be created by TreeView.
    addTestRowForEachControl(engine, QQC2_IMPORT_PATH, "basic", "QtQuick/Controls/Basic",
        QStringList() << "Calendar" << "TreeViewDelegate");
    addTestRowForEachControl(engine, QQC2_IMPORT_PATH, "fusion", "QtQuick/Controls/Fusion",
        QStringList() << "ButtonPanel" << "CheckIndicator" << "RadioIndicator" << "SliderGroove"
                             << "SliderHandle" << "SwitchIndicator" << "TreeViewDelegate");
    addTestRowForEachControl(engine, QQC2_IMPORT_PATH, "imagine", "QtQuick/Controls/Imagine");
    addTestRowForEachControl(engine, QQC2_IMPORT_PATH, "material", "QtQuick/Controls/Material",
        QStringList() << "Ripple" << "SliderHandle" << "CheckIndicator" << "RadioIndicator"
            << "SwitchIndicator" << "BoxShadow" << "ElevationEffect" << "CursorDelegate" << "TreeViewDelegate");
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
    for (QObject *object : std::as_const(*qt_qobjects())) {
        if (qobject_cast<T *>(object))
            objects += object;
    }

    if (qt_verbose) {
        for (QObject *object : objects)
            qInfo() << "\t" << object;
    }

    QTest::setBenchmarkResult(objects.size(), QTest::Events);
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
