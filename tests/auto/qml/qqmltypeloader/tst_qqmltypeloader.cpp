/****************************************************************************
**
** Copyright (C) 2016 Canonical Limited and/or its subsidiary(-ies).
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
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtQml/private/qqmlengine_p.h>
#include <QtQml/private/qqmltypeloader_p.h>
#include "../../shared/util.h"

class tst_QQMLTypeLoader : public QQmlDataTest
{
    Q_OBJECT

private slots:
    void testLoadComplete();
    void loadComponentSynchronously();
    void trimCache();
    void trimCache2();
    void keepSingleton();
    void keepRegistrations();
};

void tst_QQMLTypeLoader::testLoadComplete()
{
    QQuickView *window = new QQuickView();
    window->engine()->addImportPath(QT_TESTCASE_BUILDDIR);
    qDebug() << window->engine()->importPathList();
    window->setGeometry(0,0,240,320);
    window->setSource(testFileUrl("test_load_complete.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QObject *rootObject = window->rootObject();
    QTRY_VERIFY(rootObject != 0);
    QTRY_COMPARE(rootObject->property("created").toInt(), 2);
    QTRY_COMPARE(rootObject->property("loaded").toInt(), 2);
    delete window;
}

void tst_QQMLTypeLoader::loadComponentSynchronously()
{
    QQmlEngine engine;
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
                             QLatin1String(".*nonprotocol::1:1: QtObject is not a type.*")));
    QQmlComponent component(&engine, testFileUrl("load_synchronous.qml"));
    QObject *o = component.create();
    QVERIFY(o);
}

void tst_QQMLTypeLoader::trimCache()
{
    QQmlEngine engine;
    QQmlTypeLoader &loader = QQmlEnginePrivate::get(&engine)->typeLoader;
    for (int i = 0; i < 256; ++i) {
        QUrl url = testFileUrl("trim_cache.qml");
        url.setQuery(QString::number(i));

        QQmlTypeData *data = loader.getType(url);
        // Run an event loop to receive the callback that release()es.
        QTRY_COMPARE(data->count(), 2);

        // keep references to some of them so that they aren't trimmed. References to either the
        // QQmlTypeData or its compiledData() should prevent the trimming.
        if (i % 10 == 0) {
            // keep ref on data, don't add ref on data->compiledData()
        } else if (i % 5 == 0) {
            data->compilationUnit()->addref();
            data->release();
        } else {
            data->release();
        }
    }

    for (int i = 0; i < 256; ++i) {
        QUrl url = testFileUrl("trim_cache.qml");
        url.setQuery(QString::number(i));
        if (i % 5 == 0)
            QVERIFY(loader.isTypeLoaded(url));
        else if (i < 128)
            QVERIFY(!loader.isTypeLoaded(url));
        // The cache is free to keep the others.
    }
}

void tst_QQMLTypeLoader::trimCache2()
{
    QQuickView *window = new QQuickView();
    window->setSource(testFileUrl("trim_cache2.qml"));
    QQmlTypeLoader &loader = QQmlEnginePrivate::get(window->engine())->typeLoader;
    // in theory if gc has already run this could be false
    // QCOMPARE(loader.isTypeLoaded(testFileUrl("MyComponent2.qml")), true);
    window->engine()->collectGarbage();
    QTest::qWait(1);    // force event loop
    window->engine()->trimComponentCache();
    QCOMPARE(loader.isTypeLoaded(testFileUrl("MyComponent2.qml")), false);
}

static void checkSingleton(const QString &dataDirectory)
{
    QQmlEngine engine;
    engine.addImportPath(dataDirectory);
    QQmlComponent component(&engine);
    component.setData("import ClusterDemo 1.0\n"
                      "import QtQuick 2.6\n"
                      "import \"..\"\n"
                      "Item { property int t: ValueSource.something }",
                      QUrl::fromLocalFile(dataDirectory + "/abc/Xyz.qml"));
    QCOMPARE(component.status(), QQmlComponent::Ready);
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o.data());
    QCOMPARE(o->property("t").toInt(), 10);
}

void tst_QQMLTypeLoader::keepSingleton()
{
    qmlRegisterSingletonType(testFileUrl("ValueSource.qml"), "ClusterDemo", 1, 0, "ValueSource");
    checkSingleton(dataDirectory());
    QQmlMetaType::freeUnusedTypesAndCaches();
    checkSingleton(dataDirectory());
}

class TestObject : public QObject
{
    Q_OBJECT
public:
    TestObject(QObject *parent = 0) : QObject(parent) {}
};

QML_DECLARE_TYPE(TestObject)

static void verifyTypes(bool shouldHaveTestObject, bool shouldHaveFast)
{
    bool hasTestObject = false;
    bool hasFast = false;
    for (const QQmlType &type : QQmlMetaType::qmlAllTypes()) {
        if (type.elementName() == QLatin1String("Fast"))
            hasFast = true;
        else if (type.elementName() == QLatin1String("TestObject"))
            hasTestObject = true;
    }
    QCOMPARE(hasTestObject, shouldHaveTestObject);
    QCOMPARE(hasFast, shouldHaveFast);
}

void tst_QQMLTypeLoader::keepRegistrations()
{
    verifyTypes(false, false);
    qmlRegisterType<TestObject>("Test", 1, 0, "TestObject");
    verifyTypes(true, false);

    {
        QQmlEngine engine;
        engine.addImportPath(dataDirectory());
        QQmlComponent component(&engine);
        component.setData("import Fast 1.0\nFast {}", QUrl());
        QVERIFY2(component.errorString().isEmpty(), component.errorString().toUtf8().constData());
        QCOMPARE(component.status(), QQmlComponent::Ready);
        QScopedPointer<QObject> o(component.create());
        QVERIFY(o.data());
        verifyTypes(true, true);
    }

    verifyTypes(true, false); // Fast is gone again, even though an event was still scheduled.
    QQmlMetaType::freeUnusedTypesAndCaches();
    verifyTypes(true, false); // qmlRegisterType creates an undeletable type.
}

QTEST_MAIN(tst_QQMLTypeLoader)

#include "tst_qqmltypeloader.moc"
