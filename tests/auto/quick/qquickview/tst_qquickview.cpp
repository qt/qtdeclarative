// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtest.h>
#include <QtTest/QSignalSpy>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtGui/QWindow>
#include <QtCore/QDebug>
#include <QtQml/qqmlengine.h>

#include <QtQuickTestUtils/private/geometrytestutils_p.h>

class tst_QQuickView : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickView();

private slots:
    void resizemodeitem();
    void errors();
    void engine();
    void findChild();
    void setInitialProperties();
};


tst_QQuickView::tst_QQuickView()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickView::resizemodeitem()
{
    QWindow window;
    window.setGeometry(0, 0, 400, 400);

    QQuickView *view = new QQuickView(&window);
    QVERIFY(view);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    QCOMPARE(QSize(0,0), view->initialSize());
    view->setSource(testFileUrl("resizemodeitem.qml"));
    QQuickItem* item = qobject_cast<QQuickItem*>(view->rootObject());
    QVERIFY(item);
    window.show();

    view->showNormal();

    // initial size from root object
    QCOMPARE(item->width(), 200.0);
    QCOMPARE(item->height(), 200.0);
    QCOMPARE(view->size(), QSize(200, 200));
    QCOMPARE(view->size(), view->sizeHint());
    QCOMPARE(view->size(), view->initialSize());

    // size update from view
    view->resize(QSize(80,100));

    QTRY_COMPARE(item->width(), 80.0);
    QCOMPARE(item->height(), 100.0);
    QCOMPARE(view->size(), QSize(80, 100));
    QCOMPARE(view->size(), view->sizeHint());

    view->setResizeMode(QQuickView::SizeViewToRootObject);

    // size update from view disabled
    view->resize(QSize(60,80));
    QCOMPARE(item->width(), 80.0);
    QCOMPARE(item->height(), 100.0);
    QTest::qWait(50);
    QCOMPARE(view->size(), QSize(60, 80));

    // size update from root object
    item->setWidth(250);
    item->setHeight(350);
    QCOMPARE(item->width(), 250.0);
    QCOMPARE(item->height(), 350.0);
    QTRY_COMPARE(view->size(), QSize(250, 350));
    QCOMPARE(view->size(), QSize(250, 350));
    QCOMPARE(view->size(), view->sizeHint());

    // reset window
    window.hide();
    delete view;
    view = new QQuickView(&window);
    QVERIFY(view);
    view->setResizeMode(QQuickView::SizeViewToRootObject);
    view->setSource(testFileUrl("resizemodeitem.qml"));
    item = qobject_cast<QQuickItem*>(view->rootObject());
    QVERIFY(item);
    window.show();

    view->showNormal();

    // initial size for root object
    QCOMPARE(item->width(), 200.0);
    QCOMPARE(item->height(), 200.0);
    QCOMPARE(view->size(), view->sizeHint());
    QCOMPARE(view->size(), view->initialSize());

    // size update from root object
    item->setWidth(80);
    item->setHeight(100);
    QCOMPARE(item->width(), 80.0);
    QCOMPARE(item->height(), 100.0);
    QTRY_COMPARE(view->size(), QSize(80, 100));
    QCOMPARE(view->size(), view->sizeHint());

    // size update from root object disabled
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    item->setWidth(60);
    item->setHeight(80);
    QCOMPARE(view->width(), 80);
    QCOMPARE(view->height(), 100);
    QCOMPARE(QSize(item->width(), item->height()), view->sizeHint());

    // size update from view
    QCoreApplication::processEvents(); // make sure the last resize events are gone
    QSizeChangeListener sizeListener(item);
    view->resize(QSize(200,300));
    QTRY_COMPARE(item->width(), 200.0);

    for (int i = 0; i < sizeListener.size(); ++i) {
        // Check that we have the correct geometry on all signals
        QCOMPARE(sizeListener.at(i), view->size());
    }

    QCOMPARE(item->height(), 300.0);
    QCOMPARE(view->size(), QSize(200, 300));
    QCOMPARE(view->size(), view->sizeHint());

    window.hide();
    delete view;

    // if we set a specific size for the view then it should keep that size
    // for SizeRootObjectToView mode.
    view = new QQuickView(&window);
    view->resize(300, 300);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    QCOMPARE(QSize(0,0), view->initialSize());
    view->setSource(testFileUrl("resizemodeitem.qml"));
    view->resize(300, 300);
    item = qobject_cast<QQuickItem*>(view->rootObject());
    QVERIFY(item);
    window.show();

    view->showNormal();
    QTest::qWait(50);

    // initial size from root object
    QCOMPARE(item->width(), 300.0);
    QCOMPARE(item->height(), 300.0);
    QCOMPARE(view->size(), QSize(300, 300));
    QCOMPARE(view->size(), view->sizeHint());
    QCOMPARE(view->initialSize(), QSize(200, 200)); // initial object size

    delete view;
}

void tst_QQuickView::errors()
{
    {
        QQuickView view;
        QVERIFY(view.errors().isEmpty()); // don't crash
    }
    {
        QQuickView view;
        QQmlTestMessageHandler messageHandler;
        view.setSource(testFileUrl("error1.qml"));
        QCOMPARE(view.status(), QQuickView::Error);
        QCOMPARE(view.errors().size(), 1);
    }

    {
        QQuickView view;
        QQmlTestMessageHandler messageHandler;
        view.setSource(testFileUrl("error2.qml"));
        QCOMPARE(view.status(), QQuickView::Error);
        QCOMPARE(view.errors().size(), 1);
        view.show();
    }
}

void tst_QQuickView::engine()
{
    QScopedPointer<QQmlEngine> engine(new QQmlEngine);
    QVERIFY(!engine->incubationController());

    QScopedPointer<QQuickView> view(new QQuickView(engine.get(), nullptr));
    QVERIFY(view);
    QCOMPARE(engine->incubationController(), view->incubationController());

    QScopedPointer<QQuickView> view2(new QQuickView(engine.get(), nullptr));
    QVERIFY(view);
    QCOMPARE(engine->incubationController(), view->incubationController());
    view.reset();
    QVERIFY(!engine->incubationController());

    engine->setIncubationController(view2->incubationController());
    QCOMPARE(engine->incubationController(), view2->incubationController());
    view2.reset();
    QVERIFY(!engine->incubationController());

    QScopedPointer<QQuickView> view3(new QQuickView);
    QScopedPointer<QQuickView> view4(new QQuickView(view3->engine(), nullptr));

    QVERIFY(view3->engine());
    QVERIFY(view4->engine());
    QCOMPARE(view3->engine(), view4->engine());
    view3.reset();
    QVERIFY(!view4->engine());
    QTest::ignoreMessage(QtWarningMsg, "QQuickView: invalid qml engine.");
    view4->setSource(QUrl());

    QCOMPARE(view4->status(), QQuickView::Error);
    QVERIFY(!view4->errors().isEmpty());
    QCOMPARE(view4->errors().back().description(), QLatin1String("QQuickView: invalid qml engine."));
}

void tst_QQuickView::findChild()
{
    QQuickView view;
    view.setSource(testFileUrl("findChild.qml"));

    // QQuickView
    // |_ QQuickWindow::contentItem
    // |  |_ QQuickView::rootObject: QML Item("rootObject") (findChild.qml)
    // |  |  |_ QML Item("rootObjectChild") (findChild.qml)
    // |  |_ QObject("contentItemChild")
    // |_ QObject("viewChild")

    QObject *viewChild = new QObject(&view);
    viewChild->setObjectName("viewChild");

    QObject *contentItemChild = new QObject(view.contentItem());
    contentItemChild->setObjectName("contentItemChild");

    QObject *rootObject = view.rootObject();
    QVERIFY(rootObject);

    QObject *rootObjectChild = rootObject->findChild<QObject *>("rootObjectChild");
    QVERIFY(rootObjectChild);

    QCOMPARE(view.findChild<QObject *>("viewChild"), viewChild);
    QCOMPARE(view.findChild<QObject *>("contentItemChild"), contentItemChild);
    QCOMPARE(view.findChild<QObject *>("rootObject"), rootObject);
    QCOMPARE(view.findChild<QObject *>("rootObjectChild"), rootObjectChild);

    QVERIFY(!view.contentItem()->findChild<QObject *>("viewChild")); // sibling
    QCOMPARE(view.contentItem()->findChild<QObject *>("contentItemChild"), contentItemChild);
    QCOMPARE(view.contentItem()->findChild<QObject *>("rootObject"), rootObject);
    QCOMPARE(view.contentItem()->findChild<QObject *>("rootObjectChild"), rootObjectChild);

    QVERIFY(!view.rootObject()->findChild<QObject *>("viewChild")); // ancestor
    QVERIFY(!view.rootObject()->findChild<QObject *>("contentItemChild")); // cousin
    QVERIFY(!view.rootObject()->findChild<QObject *>("rootObject")); // self
}

void tst_QQuickView::setInitialProperties()
{
    QQuickView view;
    view.setInitialProperties({{"z", 4}, {"width", 100}});
    view.setSource(testFileUrl("resizemodeitem.qml"));
    QObject *rootObject = view.rootObject();
    QVERIFY(rootObject);
    QCOMPARE(rootObject->property("z").toInt(), 4);
    QCOMPARE(rootObject->property("width").toInt(), 100);
}

QTEST_MAIN(tst_QQuickView)

#include "tst_qquickview.moc"
