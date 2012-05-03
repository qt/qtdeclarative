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
#include <QtTest/QSignalSpy>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include "../../shared/util.h"
#include <QtGui/QWindow>
#include <QtCore/QDebug>
#include <QtQml/qqmlengine.h>

class tst_QQuickView : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickView();

private slots:
    void resizemodeitem();
    void errors();
    void engine();
};


tst_QQuickView::tst_QQuickView()
{
}

void tst_QQuickView::resizemodeitem()
{
    QWindow window;
    window.setGeometry(0, 0, 400, 400);

    QQuickView *canvas = new QQuickView(&window);
    QVERIFY(canvas);
    canvas->setResizeMode(QQuickView::SizeRootObjectToView);
    QCOMPARE(QSize(0,0), canvas->initialSize());
    canvas->setSource(testFileUrl("resizemodeitem.qml"));
    QQuickItem* item = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(item);
    window.show();

    canvas->show();

    // initial size from root object
    QCOMPARE(item->width(), 200.0);
    QCOMPARE(item->height(), 200.0);
    QCOMPARE(canvas->size(), QSize(200, 200));
    QCOMPARE(canvas->size(), canvas->sizeHint());
    QCOMPARE(canvas->size(), canvas->initialSize());

    // size update from view
    canvas->resize(QSize(80,100));

    QTRY_COMPARE(item->width(), 80.0);
    QCOMPARE(item->height(), 100.0);
    QCOMPARE(canvas->size(), QSize(80, 100));
    QCOMPARE(canvas->size(), canvas->sizeHint());

    canvas->setResizeMode(QQuickView::SizeViewToRootObject);

    // size update from view disabled
    canvas->resize(QSize(60,80));
    QCOMPARE(item->width(), 80.0);
    QCOMPARE(item->height(), 100.0);
    QTest::qWait(50);
    QCOMPARE(canvas->size(), QSize(60, 80));

    // size update from root object
    item->setWidth(250);
    item->setHeight(350);
    QCOMPARE(item->width(), 250.0);
    QCOMPARE(item->height(), 350.0);
    QTRY_COMPARE(canvas->size(), QSize(250, 350));
    QCOMPARE(canvas->size(), QSize(250, 350));
    QCOMPARE(canvas->size(), canvas->sizeHint());

    // reset canvas
    window.hide();
    delete canvas;
    canvas = new QQuickView(&window);
    QVERIFY(canvas);
    canvas->setResizeMode(QQuickView::SizeViewToRootObject);
    canvas->setSource(testFileUrl("resizemodeitem.qml"));
    item = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(item);
    window.show();

    canvas->show();

    // initial size for root object
    QCOMPARE(item->width(), 200.0);
    QCOMPARE(item->height(), 200.0);
    QCOMPARE(canvas->size(), canvas->sizeHint());
    QCOMPARE(canvas->size(), canvas->initialSize());

    // size update from root object
    item->setWidth(80);
    item->setHeight(100);
    QCOMPARE(item->width(), 80.0);
    QCOMPARE(item->height(), 100.0);
    QTRY_COMPARE(canvas->size(), QSize(80, 100));
    QCOMPARE(canvas->size(), QSize(80, 100));
    QCOMPARE(canvas->size(), canvas->sizeHint());

    // size update from root object disabled
    canvas->setResizeMode(QQuickView::SizeRootObjectToView);
    item->setWidth(60);
    item->setHeight(80);
    QCOMPARE(canvas->width(), 80);
    QCOMPARE(canvas->height(), 100);
    QCOMPARE(QSize(item->width(), item->height()), canvas->sizeHint());

    // size update from view
    canvas->resize(QSize(200,300));
    QTest::qWait(50);
    QCOMPARE(item->width(), 200.0);
    QCOMPARE(item->height(), 300.0);
    QCOMPARE(canvas->size(), QSize(200, 300));
    QCOMPARE(canvas->size(), canvas->sizeHint());

    window.hide();
    delete canvas;

    // if we set a specific size for the view then it should keep that size
    // for SizeRootObjectToView mode.
    canvas = new QQuickView(&window);
    canvas->resize(300, 300);
    canvas->setResizeMode(QQuickView::SizeRootObjectToView);
    QCOMPARE(QSize(0,0), canvas->initialSize());
    canvas->setSource(testFileUrl("resizemodeitem.qml"));
    canvas->resize(300, 300);
    item = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(item);
    window.show();

    canvas->show();
    QTest::qWait(50);

    // initial size from root object
    QCOMPARE(item->width(), 300.0);
    QCOMPARE(item->height(), 300.0);
    QCOMPARE(canvas->size(), QSize(300, 300));
    QCOMPARE(canvas->size(), canvas->sizeHint());
    QCOMPARE(canvas->initialSize(), QSize(200, 200)); // initial object size

    delete canvas;
}

static void silentErrorsMsgHandler(QtMsgType, const char *)
{
}

void tst_QQuickView::errors()
{
    QQuickView *canvas = new QQuickView;
    QVERIFY(canvas);
    QtMsgHandler old = qInstallMsgHandler(silentErrorsMsgHandler);
    canvas->setSource(testFileUrl("error1.qml"));
    qInstallMsgHandler(old);
    QVERIFY(canvas->status() == QQuickView::Error);
    QVERIFY(canvas->errors().count() == 1);
    delete canvas;
}

void tst_QQuickView::engine()
{
    QQmlEngine *engine = new QQmlEngine;
    QVERIFY(!engine->incubationController());

    QQuickView *canvas = new QQuickView(engine, 0);
    QVERIFY(canvas);
    QVERIFY(engine->incubationController() == canvas->incubationController());

    QQuickView *canvas2 = new QQuickView(engine, 0);
    QVERIFY(canvas);
    QVERIFY(engine->incubationController() == canvas->incubationController());
    delete canvas;
    QVERIFY(!engine->incubationController());

    engine->setIncubationController(canvas2->incubationController());
    QVERIFY(engine->incubationController() == canvas2->incubationController());
    delete canvas2;
    QVERIFY(!engine->incubationController());

    QQuickView *canvas3 = new QQuickView;
    QQuickView *canvas4 = new QQuickView(canvas3->engine(), 0);

    QVERIFY(canvas3->engine());
    QVERIFY(canvas4->engine());
    QCOMPARE(canvas3->engine(), canvas4->engine());
    delete canvas3;
    QVERIFY(!canvas4->engine());
    QTest::ignoreMessage(QtWarningMsg, "QQuickView: invalid qml engine. ");
    canvas4->setSource(QUrl());

    QCOMPARE(canvas4->status(), QQuickView::Error);
    QVERIFY(!canvas4->errors().isEmpty());
    QCOMPARE(canvas4->errors().back().description(), QLatin1String("QQuickView: invalid qml engine."));
    delete canvas4;
}

QTEST_MAIN(tst_QQuickView)

#include "tst_qquickview.moc"
