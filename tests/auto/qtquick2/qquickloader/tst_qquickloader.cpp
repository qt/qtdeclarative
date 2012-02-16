/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qtest.h>

#include <QSignalSpy>

#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtDeclarative/qdeclarativeincubator.h>
#include <private/qquickloader_p.h>
#include "testhttpserver.h"
#include "../../shared/util.h"

#define SERVER_PORT 14450

class PeriodicIncubationController : public QObject,
    public QDeclarativeIncubationController
{
public:
    PeriodicIncubationController() {
        startTimer(16);
    }

protected:
    virtual void timerEvent(QTimerEvent *) {
        incubateFor(15);
    }
};

class tst_QQuickLoader : public QDeclarativeDataTest

{
    Q_OBJECT
public:
    tst_QQuickLoader();

private slots:
    void sourceOrComponent();
    void sourceOrComponent_data();
    void clear();
    void urlToComponent();
    void componentToUrl();
    void anchoredLoader();
    void sizeLoaderToItem();
    void sizeItemToLoader();
    void noResize();
    void networkRequestUrl();
    void failNetworkRequest();
//    void networkComponent();
    void active();
    void initialPropertyValues_data();
    void initialPropertyValues();
    void initialPropertyValuesBinding();
    void initialPropertyValuesError_data();
    void initialPropertyValuesError();

    void deleteComponentCrash();
    void nonItem();
    void vmeErrors();
    void creationContext();
    void QTBUG_16928();
    void implicitSize();
    void QTBUG_17114();
    void asynchronous_data();
    void asynchronous();
    void asynchronous_clear();

    void parented();
    void sizeBound();

private:
    QDeclarativeEngine engine;
};


tst_QQuickLoader::tst_QQuickLoader()
{
}

void tst_QQuickLoader::sourceOrComponent()
{
    QFETCH(QString, sourceOrComponent);
    QFETCH(QString, sourceDefinition);
    QFETCH(QUrl, sourceUrl);
    QFETCH(QString, errorString);

    bool error = !errorString.isEmpty();
    if (error)
        QTest::ignoreMessage(QtWarningMsg, errorString.toUtf8().constData());

    QDeclarativeComponent component(&engine);
    component.setData(QByteArray(
            "import QtQuick 2.0\n"
            "Loader {\n"
            "   property int onItemChangedCount: 0\n"
            "   property int onSourceChangedCount: 0\n"
            "   property int onSourceComponentChangedCount: 0\n"
            "   property int onStatusChangedCount: 0\n"
            "   property int onProgressChangedCount: 0\n"
            "   property int onLoadedCount: 0\n")
            + sourceDefinition.toUtf8()
            + QByteArray(
            "   onItemChanged: onItemChangedCount += 1\n"
            "   onSourceChanged: onSourceChangedCount += 1\n"
            "   onSourceComponentChanged: onSourceComponentChangedCount += 1\n"
            "   onStatusChanged: onStatusChangedCount += 1\n"
            "   onProgressChanged: onProgressChangedCount += 1\n"
            "   onLoaded: onLoadedCount += 1\n"
            "}")
        , dataDirectoryUrl());

    QQuickLoader *loader = qobject_cast<QQuickLoader*>(component.create());
    QVERIFY(loader != 0);
    QCOMPARE(loader->item() == 0, error);
    QCOMPARE(loader->source(), sourceUrl);
    QCOMPARE(loader->progress(), 1.0);

    QCOMPARE(loader->status(), error ? QQuickLoader::Error : QQuickLoader::Ready);
    QCOMPARE(static_cast<QQuickItem*>(loader)->childItems().count(), error ? 0: 1);

    if (!error) {
        bool sourceComponentIsChildOfLoader = false;
        for (int ii = 0; ii < loader->children().size(); ++ii) {
            QDeclarativeComponent *c = qobject_cast<QDeclarativeComponent*>(loader->children().at(ii));
            if (c && c == loader->sourceComponent()) {
                sourceComponentIsChildOfLoader = true;
            }
        }
        QVERIFY(sourceComponentIsChildOfLoader);
    }

    if (sourceOrComponent == "component") {
        QCOMPARE(loader->property("onSourceComponentChangedCount").toInt(), 1);
        QCOMPARE(loader->property("onSourceChangedCount").toInt(), 0);
    } else {
        QCOMPARE(loader->property("onSourceComponentChangedCount").toInt(), 0);
        QCOMPARE(loader->property("onSourceChangedCount").toInt(), 1);
    }
    QCOMPARE(loader->property("onStatusChangedCount").toInt(), 1);
    QCOMPARE(loader->property("onProgressChangedCount").toInt(), 1);

    QCOMPARE(loader->property("onItemChangedCount").toInt(), error ? 0 : 1);
    QCOMPARE(loader->property("onLoadedCount").toInt(), error ? 0 : 1);

    delete loader;
}

void tst_QQuickLoader::sourceOrComponent_data()
{
    QTest::addColumn<QString>("sourceOrComponent");
    QTest::addColumn<QString>("sourceDefinition");
    QTest::addColumn<QUrl>("sourceUrl");
    QTest::addColumn<QString>("errorString");

    QTest::newRow("source") << "source" << "source: 'Rect120x60.qml'\n" << testFileUrl("Rect120x60.qml") << "";
    QTest::newRow("sourceComponent") << "component" << "Component { id: comp; Rectangle { width: 100; height: 50 } }\n sourceComponent: comp\n" << QUrl() << "";
    QTest::newRow("invalid source") << "source" << "source: 'IDontExist.qml'\n" << testFileUrl("IDontExist.qml")
            << QString(testFileUrl("IDontExist.qml").toString() + ": File not found");
}

void tst_QQuickLoader::clear()
{
    {
        QDeclarativeComponent component(&engine);
        component.setData(QByteArray(
                    "import QtQuick 2.0\n"
                    " Loader { id: loader\n"
                    "  source: 'Rect120x60.qml'\n"
                    "  Timer { interval: 200; running: true; onTriggered: loader.source = '' }\n"
                    " }")
                , dataDirectoryUrl());
        QQuickLoader *loader = qobject_cast<QQuickLoader*>(component.create());
        QVERIFY(loader != 0);
        QVERIFY(loader->item());
        QCOMPARE(loader->progress(), 1.0);
        QCOMPARE(static_cast<QQuickItem*>(loader)->childItems().count(), 1);

        QTRY_VERIFY(loader->item() == 0);
        QCOMPARE(loader->progress(), 0.0);
        QCOMPARE(loader->status(), QQuickLoader::Null);
        QCOMPARE(static_cast<QQuickItem*>(loader)->childItems().count(), 0);

        delete loader;
    }
    {
        QDeclarativeComponent component(&engine, testFileUrl("/SetSourceComponent.qml"));
        QQuickItem *item = qobject_cast<QQuickItem*>(component.create());
        QVERIFY(item);

        QQuickLoader *loader = qobject_cast<QQuickLoader*>(item->QQuickItem::childItems().at(0));
        QVERIFY(loader);
        QVERIFY(loader->item());
        QCOMPARE(loader->progress(), 1.0);
        QCOMPARE(static_cast<QQuickItem*>(loader)->childItems().count(), 1);

        loader->setSourceComponent(0);

        QVERIFY(loader->item() == 0);
        QCOMPARE(loader->progress(), 0.0);
        QCOMPARE(loader->status(), QQuickLoader::Null);
        QCOMPARE(static_cast<QQuickItem*>(loader)->childItems().count(), 0);

        delete item;
    }
    {
        QDeclarativeComponent component(&engine, testFileUrl("/SetSourceComponent.qml"));
        QQuickItem *item = qobject_cast<QQuickItem*>(component.create());
        QVERIFY(item);

        QQuickLoader *loader = qobject_cast<QQuickLoader*>(item->QQuickItem::childItems().at(0));
        QVERIFY(loader);
        QVERIFY(loader->item());
        QCOMPARE(loader->progress(), 1.0);
        QCOMPARE(static_cast<QQuickItem*>(loader)->childItems().count(), 1);

        QMetaObject::invokeMethod(item, "clear");

        QVERIFY(loader->item() == 0);
        QCOMPARE(loader->progress(), 0.0);
        QCOMPARE(loader->status(), QQuickLoader::Null);
        QCOMPARE(static_cast<QQuickItem*>(loader)->childItems().count(), 0);

        delete item;
    }
}

void tst_QQuickLoader::urlToComponent()
{
    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import QtQuick 2.0\n"
                "Loader {\n"
                " id: loader\n"
                " Component { id: myComp; Rectangle { width: 10; height: 10 } }\n"
                " source: \"Rect120x60.qml\"\n"
                " Timer { interval: 100; running: true; onTriggered: loader.sourceComponent = myComp }\n"
                "}" )
            , dataDirectoryUrl());
    QQuickLoader *loader = qobject_cast<QQuickLoader*>(component.create());
    QTest::qWait(200);
    QTRY_VERIFY(loader != 0);
    QVERIFY(loader->item());
    QCOMPARE(loader->progress(), 1.0);
    QCOMPARE(static_cast<QQuickItem*>(loader)->childItems().count(), 1);
    QCOMPARE(loader->width(), 10.0);
    QCOMPARE(loader->height(), 10.0);

    delete loader;
}

void tst_QQuickLoader::componentToUrl()
{
    QDeclarativeComponent component(&engine, testFileUrl("/SetSourceComponent.qml"));
    QQuickItem *item = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(item);

    QQuickLoader *loader = qobject_cast<QQuickLoader*>(item->QQuickItem::childItems().at(0));
    QVERIFY(loader);
    QVERIFY(loader->item());
    QCOMPARE(loader->progress(), 1.0);
    QCOMPARE(static_cast<QQuickItem*>(loader)->childItems().count(), 1);

    loader->setSource(testFileUrl("/Rect120x60.qml"));
    QVERIFY(loader->item());
    QCOMPARE(loader->progress(), 1.0);
    QCOMPARE(static_cast<QQuickItem*>(loader)->childItems().count(), 1);
    QCOMPARE(loader->width(), 120.0);
    QCOMPARE(loader->height(), 60.0);

    delete item;
}

void tst_QQuickLoader::anchoredLoader()
{
    QDeclarativeComponent component(&engine, testFileUrl("/AnchoredLoader.qml"));
    QQuickItem *rootItem = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(rootItem != 0);
    QQuickItem *loader = rootItem->findChild<QQuickItem*>("loader");
    QQuickItem *sourceElement = rootItem->findChild<QQuickItem*>("sourceElement");

    QVERIFY(loader != 0);
    QVERIFY(sourceElement != 0);

    QCOMPARE(rootItem->width(), 300.0);
    QCOMPARE(rootItem->height(), 200.0);

    QCOMPARE(loader->width(), 300.0);
    QCOMPARE(loader->height(), 200.0);

    QCOMPARE(sourceElement->width(), 300.0);
    QCOMPARE(sourceElement->height(), 200.0);
}

void tst_QQuickLoader::sizeLoaderToItem()
{
    QDeclarativeComponent component(&engine, testFileUrl("/SizeToItem.qml"));
    QQuickLoader *loader = qobject_cast<QQuickLoader*>(component.create());
    QVERIFY(loader != 0);
    QCOMPARE(loader->width(), 120.0);
    QCOMPARE(loader->height(), 60.0);

    // Check resize
    QQuickItem *rect = qobject_cast<QQuickItem*>(loader->item());
    QVERIFY(rect);
    rect->setWidth(150);
    rect->setHeight(45);
    QCOMPARE(loader->width(), 150.0);
    QCOMPARE(loader->height(), 45.0);

    // Check explicit width
    loader->setWidth(200.0);
    QCOMPARE(loader->width(), 200.0);
    QCOMPARE(rect->width(), 200.0);
    rect->setWidth(100.0); // when rect changes ...
    QCOMPARE(rect->width(), 100.0); // ... it changes
    QCOMPARE(loader->width(), 200.0); // ... but loader stays the same

    // Check explicit height
    loader->setHeight(200.0);
    QCOMPARE(loader->height(), 200.0);
    QCOMPARE(rect->height(), 200.0);
    rect->setHeight(100.0); // when rect changes ...
    QCOMPARE(rect->height(), 100.0); // ... it changes
    QCOMPARE(loader->height(), 200.0); // ... but loader stays the same

    // Switch mode
    loader->setWidth(180);
    loader->setHeight(30);
    QCOMPARE(rect->width(), 180.0);
    QCOMPARE(rect->height(), 30.0);

    delete loader;
}

void tst_QQuickLoader::sizeItemToLoader()
{
    QDeclarativeComponent component(&engine, testFileUrl("/SizeToLoader.qml"));
    QQuickLoader *loader = qobject_cast<QQuickLoader*>(component.create());
    QVERIFY(loader != 0);
    QCOMPARE(loader->width(), 200.0);
    QCOMPARE(loader->height(), 80.0);

    QQuickItem *rect = qobject_cast<QQuickItem*>(loader->item());
    QVERIFY(rect);
    QCOMPARE(rect->width(), 200.0);
    QCOMPARE(rect->height(), 80.0);

    // Check resize
    loader->setWidth(180);
    loader->setHeight(30);
    QCOMPARE(rect->width(), 180.0);
    QCOMPARE(rect->height(), 30.0);

    // Switch mode
    loader->resetWidth(); // reset explicit size
    loader->resetHeight();
    rect->setWidth(160);
    rect->setHeight(45);
    QCOMPARE(loader->width(), 160.0);
    QCOMPARE(loader->height(), 45.0);

    delete loader;
}

void tst_QQuickLoader::noResize()
{
    QDeclarativeComponent component(&engine, testFileUrl("/NoResize.qml"));
    QQuickItem* item = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(item != 0);
    QCOMPARE(item->width(), 200.0);
    QCOMPARE(item->height(), 80.0);

    delete item;
}

void tst_QQuickLoader::networkRequestUrl()
{
    TestHTTPServer server(SERVER_PORT);
    QVERIFY(server.isValid());
    server.serveDirectory(dataDirectory());

    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import QtQuick 2.0\nLoader { property int signalCount : 0; source: \"http://127.0.0.1:14450/Rect120x60.qml\"; onLoaded: signalCount += 1 }"), testFileUrl("../dummy.qml"));
    if (component.isError())
        qDebug() << component.errors();
    QQuickLoader *loader = qobject_cast<QQuickLoader*>(component.create());
    QVERIFY(loader != 0);

    QTRY_VERIFY(loader->status() == QQuickLoader::Ready);

    QVERIFY(loader->item());
    QCOMPARE(loader->progress(), 1.0);
    QCOMPARE(loader->property("signalCount").toInt(), 1);
    QCOMPARE(static_cast<QQuickItem*>(loader)->childItems().count(), 1);

    delete loader;
}

/* XXX Component waits until all dependencies are loaded.  Is this actually possible?
void tst_QQuickLoader::networkComponent()
{
    TestHTTPServer server(SERVER_PORT);
    QVERIFY(server.isValid());
    server.serveDirectory("slowdata", TestHTTPServer::Delay);

    QDeclarativeComponent component(&engine);
    component.setData(QByteArray(
                "import QtQuick 2.0\n"
                "import \"http://127.0.0.1:14450/\" as NW\n"
                "Item {\n"
                " Component { id: comp; NW.SlowRect {} }\n"
                " Loader { sourceComponent: comp } }")
            , dataDirectoryUrl());

    QQuickItem *item = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(item);

    QQuickLoader *loader = qobject_cast<QQuickLoader*>(item->QQuickItem::children().at(1));
    QVERIFY(loader);
    QTRY_VERIFY(loader->status() == QQuickLoader::Ready);

    QVERIFY(loader->item());
    QCOMPARE(loader->progress(), 1.0);
    QCOMPARE(loader->status(), QQuickLoader::Ready);
    QCOMPARE(static_cast<QQuickItem*>(loader)->children().count(), 1);

    delete loader;
}
*/

void tst_QQuickLoader::failNetworkRequest()
{
    TestHTTPServer server(SERVER_PORT);
    QVERIFY(server.isValid());
    server.serveDirectory(dataDirectory());

    QTest::ignoreMessage(QtWarningMsg, "http://127.0.0.1:14450/IDontExist.qml: File not found");

    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import QtQuick 2.0\nLoader { property int did_load: 123; source: \"http://127.0.0.1:14450/IDontExist.qml\"; onLoaded: did_load=456 }"), QUrl::fromLocalFile("http://127.0.0.1:14450/dummy.qml"));
    QQuickLoader *loader = qobject_cast<QQuickLoader*>(component.create());
    QVERIFY(loader != 0);

    QTRY_VERIFY(loader->status() == QQuickLoader::Error);

    QVERIFY(loader->item() == 0);
    QCOMPARE(loader->progress(), 0.0);
    QCOMPARE(loader->property("did_load").toInt(), 123);
    QCOMPARE(static_cast<QQuickItem*>(loader)->childItems().count(), 0);

    delete loader;
}

void tst_QQuickLoader::active()
{
    // check that the item isn't instantiated until active is set to true
    {
        QDeclarativeComponent component(&engine, testFileUrl("active.1.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        QQuickLoader *loader = object->findChild<QQuickLoader*>("loader");

        QVERIFY(loader->active() == false); // set manually to false
        QVERIFY(loader->item() == 0);
        QMetaObject::invokeMethod(object, "doSetSourceComponent");
        QVERIFY(loader->item() == 0);
        QMetaObject::invokeMethod(object, "doSetSource");
        QVERIFY(loader->item() == 0);
        QMetaObject::invokeMethod(object, "doSetActive");
        QVERIFY(loader->item() != 0);

        delete object;
    }

    // check that the status is Null if active is set to false
    {
        QDeclarativeComponent component(&engine, testFileUrl("active.2.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        QQuickLoader *loader = object->findChild<QQuickLoader*>("loader");

        QVERIFY(loader->active() == true); // active is true by default
        QCOMPARE(loader->status(), QQuickLoader::Ready);
        int currStatusChangedCount = loader->property("statusChangedCount").toInt();
        QMetaObject::invokeMethod(object, "doSetInactive");
        QCOMPARE(loader->status(), QQuickLoader::Null);
        QCOMPARE(loader->property("statusChangedCount").toInt(), (currStatusChangedCount+1));

        delete object;
    }

    // check that the source is not cleared if active is set to false
    {
        QDeclarativeComponent component(&engine, testFileUrl("active.3.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        QQuickLoader *loader = object->findChild<QQuickLoader*>("loader");

        QVERIFY(loader->active() == true); // active is true by default
        QVERIFY(!loader->source().isEmpty());
        int currSourceChangedCount = loader->property("sourceChangedCount").toInt();
        QMetaObject::invokeMethod(object, "doSetInactive");
        QVERIFY(!loader->source().isEmpty());
        QCOMPARE(loader->property("sourceChangedCount").toInt(), currSourceChangedCount);

        delete object;
    }

    // check that the sourceComponent is not cleared if active is set to false
    {
        QDeclarativeComponent component(&engine, testFileUrl("active.4.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        QQuickLoader *loader = object->findChild<QQuickLoader*>("loader");

        QVERIFY(loader->active() == true); // active is true by default
        QVERIFY(loader->sourceComponent() != 0);
        int currSourceComponentChangedCount = loader->property("sourceComponentChangedCount").toInt();
        QMetaObject::invokeMethod(object, "doSetInactive");
        QVERIFY(loader->sourceComponent() != 0);
        QCOMPARE(loader->property("sourceComponentChangedCount").toInt(), currSourceComponentChangedCount);

        delete object;
    }

    // check that the item is released if active is set to false
    {
        QDeclarativeComponent component(&engine, testFileUrl("active.5.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        QQuickLoader *loader = object->findChild<QQuickLoader*>("loader");

        QVERIFY(loader->active() == true); // active is true by default
        QVERIFY(loader->item() != 0);
        int currItemChangedCount = loader->property("itemChangedCount").toInt();
        QMetaObject::invokeMethod(object, "doSetInactive");
        QVERIFY(loader->item() == 0);
        QCOMPARE(loader->property("itemChangedCount").toInt(), (currItemChangedCount+1));

        delete object;
    }

    // check that the activeChanged signal is emitted correctly
    {
        QDeclarativeComponent component(&engine, testFileUrl("active.6.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        QQuickLoader *loader = object->findChild<QQuickLoader*>("loader");

        QVERIFY(loader->active() == true); // active is true by default
        loader->setActive(true);           // no effect
        QCOMPARE(loader->property("activeChangedCount").toInt(), 0);
        loader->setActive(false);          // change signal should be emitted
        QCOMPARE(loader->property("activeChangedCount").toInt(), 1);
        loader->setActive(false);          // no effect
        QCOMPARE(loader->property("activeChangedCount").toInt(), 1);
        loader->setActive(true);           // change signal should be emitted
        QCOMPARE(loader->property("activeChangedCount").toInt(), 2);
        loader->setActive(false);          // change signal should be emitted
        QCOMPARE(loader->property("activeChangedCount").toInt(), 3);
        QMetaObject::invokeMethod(object, "doSetActive");
        QCOMPARE(loader->property("activeChangedCount").toInt(), 4);
        QMetaObject::invokeMethod(object, "doSetActive");
        QCOMPARE(loader->property("activeChangedCount").toInt(), 4);
        QMetaObject::invokeMethod(object, "doSetInactive");
        QCOMPARE(loader->property("activeChangedCount").toInt(), 5);
        loader->setActive(true);           // change signal should be emitted
        QCOMPARE(loader->property("activeChangedCount").toInt(), 6);

        delete object;
    }

    // check that the component isn't loaded until active is set to true
    {
        QDeclarativeComponent component(&engine, testFileUrl("active.7.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        QCOMPARE(object->property("success").toBool(), true);
        delete object;
    }

    // check that the component is loaded if active is not set (true by default)
    {
        QDeclarativeComponent component(&engine, testFileUrl("active.8.qml"));
        QObject *object = component.create();
        QVERIFY(object != 0);
        QCOMPARE(object->property("success").toBool(), true);
        delete object;
    }
}

void tst_QQuickLoader::initialPropertyValues_data()
{
    QTest::addColumn<QUrl>("qmlFile");
    QTest::addColumn<QStringList>("expectedWarnings");
    QTest::addColumn<QStringList>("propertyNames");
    QTest::addColumn<QVariantList>("propertyValues");

    QTest::newRow("source url with value set in onLoaded, initially active = true") << testFileUrl("initialPropertyValues.1.qml")
            << QStringList()
            << (QStringList() << "initialValue" << "behaviorCount")
            << (QVariantList() << 1 << 1);

    QTest::newRow("set source with initial property values specified, active = true") << testFileUrl("initialPropertyValues.2.qml")
            << QStringList()
            << (QStringList() << "initialValue" << "behaviorCount")
            << (QVariantList() << 2 << 0);

    QTest::newRow("set source with initial property values specified, active = false") << testFileUrl("initialPropertyValues.3.qml")
            << (QStringList() << QString(QLatin1String("file://") + testFileUrl("initialPropertyValues.3.qml").toLocalFile() + QLatin1String(":16: TypeError: Cannot read property 'canary' of null")))
            << (QStringList())
            << (QVariantList());

    QTest::newRow("set source with initial property values specified, active = false, with active set true later") << testFileUrl("initialPropertyValues.4.qml")
            << QStringList()
            << (QStringList() << "initialValue" << "behaviorCount")
            << (QVariantList() << 4 << 0);

    QTest::newRow("set source without initial property values specified, active = true") << testFileUrl("initialPropertyValues.5.qml")
            << QStringList()
            << (QStringList() << "initialValue" << "behaviorCount")
            << (QVariantList() << 0 << 0);

    QTest::newRow("set source with initial property values specified with binding, active = true") << testFileUrl("initialPropertyValues.6.qml")
            << QStringList()
            << (QStringList() << "initialValue" << "behaviorCount")
            << (QVariantList() << 6 << 0);

    QTest::newRow("ensure initial property value semantics mimic createObject") << testFileUrl("initialPropertyValues.7.qml")
            << QStringList()
            << (QStringList() << "loaderValue" << "createObjectValue")
            << (QVariantList() << 1 << 1);

    QTest::newRow("ensure initial property values aren't disposed prior to component completion") << testFileUrl("initialPropertyValues.8.qml")
            << QStringList()
            << (QStringList() << "initialValue")
            << (QVariantList() << 6);
}

void tst_QQuickLoader::initialPropertyValues()
{
    QFETCH(QUrl, qmlFile);
    QFETCH(QStringList, expectedWarnings);
    QFETCH(QStringList, propertyNames);
    QFETCH(QVariantList, propertyValues);

    TestHTTPServer server(SERVER_PORT);
    QVERIFY(server.isValid());
    server.serveDirectory(dataDirectory());

    foreach (const QString &warning, expectedWarnings)
        QTest::ignoreMessage(QtWarningMsg, warning.toAscii().constData());

    QDeclarativeComponent component(&engine, qmlFile);
    QObject *object = component.create();
    QVERIFY(object != 0);
    qApp->processEvents();
    QTest::qWait(50);

    for (int i = 0; i < propertyNames.size(); ++i)
        QCOMPARE(object->property(propertyNames.at(i).toAscii().constData()), propertyValues.at(i));

    delete object;
}

void tst_QQuickLoader::initialPropertyValuesBinding()
{
    QDeclarativeComponent component(&engine, testFileUrl("initialPropertyValues.binding.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);

    QVERIFY(object->setProperty("bindable", QVariant(8)));
    QCOMPARE(object->property("canaryValue").toInt(), 8);

    delete object;
}

void tst_QQuickLoader::initialPropertyValuesError_data()
{
    QTest::addColumn<QUrl>("qmlFile");
    QTest::addColumn<QStringList>("expectedWarnings");

    QTest::newRow("invalid initial property values object") << testFileUrl("initialPropertyValues.error.1.qml")
            << (QStringList() << QString(testFileUrl("initialPropertyValues.error.1.qml").toString() + ":6:5: QML Loader: setSource: value is not an object"));

    QTest::newRow("nonexistent source url") << testFileUrl("initialPropertyValues.error.2.qml")
            << (QStringList() << QString(testFileUrl("NonexistentSourceComponent.qml").toString() + ": File not found"));

    QTest::newRow("invalid source url") << testFileUrl("initialPropertyValues.error.3.qml")
            << (QStringList() << QString(testFileUrl("InvalidSourceComponent.qml").toString() + ":5:1: Syntax error"));

    QTest::newRow("invalid initial property values object with invalid property access") << testFileUrl("initialPropertyValues.error.4.qml")
            << (QStringList() << QString(testFileUrl("initialPropertyValues.error.4.qml").toString() + ":7:5: QML Loader: setSource: value is not an object")
                              << QString(testFileUrl("initialPropertyValues.error.4.qml").toString() + ":5: TypeError: Cannot read property 'canary' of null"));
}

void tst_QQuickLoader::initialPropertyValuesError()
{
    QFETCH(QUrl, qmlFile);
    QFETCH(QStringList, expectedWarnings);

    foreach (const QString &warning, expectedWarnings)
        QTest::ignoreMessage(QtWarningMsg, warning.toUtf8().constData());

    QDeclarativeComponent component(&engine, qmlFile);
    QObject *object = component.create();
    QVERIFY(object != 0);
    QQuickLoader *loader = object->findChild<QQuickLoader*>("loader");
    QVERIFY(loader != 0);
    QVERIFY(loader->item() == 0);
    delete object;
}

// QTBUG-9241
void tst_QQuickLoader::deleteComponentCrash()
{
    QDeclarativeComponent component(&engine, testFileUrl("crash.qml"));
    QQuickItem *item = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(item);

    item->metaObject()->invokeMethod(item, "setLoaderSource");

    QQuickLoader *loader = qobject_cast<QQuickLoader*>(item->QQuickItem::childItems().at(0));
    QVERIFY(loader);
    QVERIFY(loader->item());
    QCOMPARE(loader->item()->objectName(), QLatin1String("blue"));
    QCOMPARE(loader->progress(), 1.0);
    QCOMPARE(loader->status(), QQuickLoader::Ready);
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QTRY_COMPARE(static_cast<QQuickItem*>(loader)->childItems().count(), 1);
    QVERIFY(loader->source() == testFileUrl("BlueRect.qml"));

    delete item;
}

void tst_QQuickLoader::nonItem()
{
    QDeclarativeComponent component(&engine, testFileUrl("nonItem.qml"));
    QString err = testFileUrl("nonItem.qml").toString() + ":3:1: QML Loader: Loader does not support loading non-visual elements.";

    QTest::ignoreMessage(QtWarningMsg, err.toLatin1().constData());
    QQuickLoader *loader = qobject_cast<QQuickLoader*>(component.create());
    QVERIFY(loader);
    QVERIFY(loader->item() == 0);

    delete loader;
}

void tst_QQuickLoader::vmeErrors()
{
    QDeclarativeComponent component(&engine, testFileUrl("vmeErrors.qml"));
    QString err = testFileUrl("VmeError.qml").toString() + ":6: Cannot assign object type QObject with no default method";
    QTest::ignoreMessage(QtWarningMsg, err.toLatin1().constData());
    QQuickLoader *loader = qobject_cast<QQuickLoader*>(component.create());
    QVERIFY(loader);
    QVERIFY(loader->item() == 0);

    delete loader;
}

// QTBUG-13481
void tst_QQuickLoader::creationContext()
{
    QDeclarativeComponent component(&engine, testFileUrl("creationContext.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test").toBool(), true);

    delete o;
}

void tst_QQuickLoader::QTBUG_16928()
{
    QDeclarativeComponent component(&engine, testFileUrl("QTBUG_16928.qml"));
    QQuickItem *item = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(item);

    QCOMPARE(item->width(), 250.);
    QCOMPARE(item->height(), 250.);

    delete item;
}

void tst_QQuickLoader::implicitSize()
{
    QDeclarativeComponent component(&engine, testFileUrl("implicitSize.qml"));
    QQuickItem *item = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(item);

    QCOMPARE(item->width(), 150.);
    QCOMPARE(item->height(), 150.);

    QCOMPARE(item->property("implHeight").toReal(), 100.);
    QCOMPARE(item->property("implWidth").toReal(), 100.);

    delete item;
}

void tst_QQuickLoader::QTBUG_17114()
{
    QDeclarativeComponent component(&engine, testFileUrl("QTBUG_17114.qml"));
    QQuickItem *item = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(item);

    QCOMPARE(item->property("loaderWidth").toReal(), 32.);
    QCOMPARE(item->property("loaderHeight").toReal(), 32.);

    delete item;
}

void tst_QQuickLoader::asynchronous_data()
{
    QTest::addColumn<QUrl>("qmlFile");
    QTest::addColumn<QStringList>("expectedWarnings");

    QTest::newRow("Valid component") << testFileUrl("BigComponent.qml")
            << QStringList();

    QTest::newRow("Non-existant component") << testFileUrl("IDoNotExist.qml")
            << (QStringList() << QString(testFileUrl("IDoNotExist.qml").toString() + ": File not found"));

    QTest::newRow("Invalid component") << testFileUrl("InvalidSourceComponent.qml")
            << (QStringList() << QString(testFileUrl("InvalidSourceComponent.qml").toString() + ":5:1: Syntax error"));
}

void tst_QQuickLoader::asynchronous()
{
    QFETCH(QUrl, qmlFile);
    QFETCH(QStringList, expectedWarnings);

    if (!engine.incubationController())
        engine.setIncubationController(new PeriodicIncubationController);
    QDeclarativeComponent component(&engine, testFileUrl("asynchronous.qml"));
    QQuickItem *root = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(root);

    QQuickLoader *loader = root->findChild<QQuickLoader*>("loader");
    QVERIFY(loader);

    foreach (const QString &warning, expectedWarnings)
        QTest::ignoreMessage(QtWarningMsg, warning.toUtf8().constData());

    QVERIFY(!loader->item());
    root->setProperty("comp", qmlFile.toString());
    QMetaObject::invokeMethod(root, "loadComponent");
    QVERIFY(!loader->item());

    if (expectedWarnings.isEmpty()) {
        QCOMPARE(loader->status(), QQuickLoader::Loading);
        QCOMPARE(engine.incubationController()->incubatingObjectCount(), 1);

        QTRY_VERIFY(loader->item());
        QCOMPARE(loader->progress(), 1.0);
        QCOMPARE(loader->status(), QQuickLoader::Ready);
    } else {
        QCOMPARE(loader->progress(), 1.0);
        QTRY_COMPARE(loader->status(), QQuickLoader::Error);
    }

    delete root;
}

void tst_QQuickLoader::asynchronous_clear()
{
    if (!engine.incubationController())
        engine.setIncubationController(new PeriodicIncubationController);
    QDeclarativeComponent component(&engine, testFileUrl("asynchronous.qml"));
    QQuickItem *root = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(root);

    QQuickLoader *loader = root->findChild<QQuickLoader*>("loader");
    QVERIFY(loader);

    QVERIFY(!loader->item());
    root->setProperty("comp", "BigComponent.qml");
    QMetaObject::invokeMethod(root, "loadComponent");
    QVERIFY(!loader->item());

    QCOMPARE(loader->status(), QQuickLoader::Loading);
    QCOMPARE(engine.incubationController()->incubatingObjectCount(), 1);

    // clear before component created
    root->setProperty("comp", "");
    QMetaObject::invokeMethod(root, "loadComponent");
    QVERIFY(!loader->item());
    QCOMPARE(engine.incubationController()->incubatingObjectCount(), 0);

    QCOMPARE(loader->progress(), 0.0);
    QCOMPARE(loader->status(), QQuickLoader::Null);
    QCOMPARE(static_cast<QQuickItem*>(loader)->childItems().count(), 0);

    // check loading component
    root->setProperty("comp", "Rect120x60.qml");
    QMetaObject::invokeMethod(root, "loadComponent");
    QVERIFY(!loader->item());

    QCOMPARE(loader->status(), QQuickLoader::Loading);
    QCOMPARE(engine.incubationController()->incubatingObjectCount(), 1);

    QTRY_VERIFY(loader->item());
    QCOMPARE(loader->progress(), 1.0);
    QCOMPARE(loader->status(), QQuickLoader::Ready);
    QCOMPARE(static_cast<QQuickItem*>(loader)->childItems().count(), 1);
}

void tst_QQuickLoader::parented()
{
    QDeclarativeComponent component(&engine, testFileUrl("parented.qml"));
    QQuickItem *root = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(root);

    QQuickItem *item = root->findChild<QQuickItem*>("comp");
    QVERIFY(item);

    QVERIFY(item->parentItem() == root);

    QCOMPARE(item->width(), 300.);
    QCOMPARE(item->height(), 300.);

    delete root;
}

void tst_QQuickLoader::sizeBound()
{
    QDeclarativeComponent component(&engine, testFileUrl("sizebound.qml"));
    QQuickItem *root = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(root);
    QQuickLoader *loader = root->findChild<QQuickLoader*>("loader");
    QVERIFY(loader != 0);

    QVERIFY(loader->item());

    QCOMPARE(loader->width(), 50.0);
    QCOMPARE(loader->height(), 60.0);

    QMetaObject::invokeMethod(root, "switchComponent");

    QCOMPARE(loader->width(), 80.0);
    QCOMPARE(loader->height(), 90.0);

    delete root;
}


QTEST_MAIN(tst_QQuickLoader)

#include "tst_qquickloader.moc"
