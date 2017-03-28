/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include <QtQuick/qquickview.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlexpression.h>
#include <QtQml/qqmlincubator.h>
#include <QtQuick/private/qquickpathitem_p.h>

#include "../../shared/util.h"
#include "../shared/viewtestutil.h"
#include "../shared/visualtestutil.h"

using namespace QQuickViewTestUtil;
using namespace QQuickVisualTestUtil;

class tst_QQuickPathItem : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickPathItem();

private slots:
    void initValues();
    void vpInitValues();
    void basicPathItem();
    void changeSignals();
    void render();
    void renderWithMultipleVp();
};

tst_QQuickPathItem::tst_QQuickPathItem()
{
    // Force the software backend to get reliable rendering results regardless of the hw and drivers.
    QQuickWindow::setSceneGraphBackend(QSGRendererInterface::Software);
}

void tst_QQuickPathItem::initValues()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("pathitem1.qml"));
    QQuickPathItem *obj = qobject_cast<QQuickPathItem *>(c.create());

    QVERIFY(obj != nullptr);
    QVERIFY(obj->rendererType() == QQuickPathItem::UnknownRenderer);
    QVERIFY(!obj->asynchronous());
    QVERIFY(obj->enableVendorExtensions());
    QVERIFY(obj->status() == QQuickPathItem::Null);
    auto vps = obj->elements();
    QVERIFY(vps.count(&vps) == 0);

    delete obj;
}

void tst_QQuickPathItem::vpInitValues()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("pathitem2.qml"));
    QQuickPathItem *obj = qobject_cast<QQuickPathItem *>(c.create());

    QVERIFY(obj != nullptr);
    QVERIFY(obj->rendererType() == QQuickPathItem::UnknownRenderer);
    QVERIFY(!obj->asynchronous());
    QVERIFY(obj->enableVendorExtensions());
    QVERIFY(obj->status() == QQuickPathItem::Null);
    auto vps = obj->elements();
    QVERIFY(vps.count(&vps) == 2);

    QQuickVisualPath *vp = vps.at(&vps, 0);
    QVERIFY(vp != nullptr);
    QVERIFY(!vp->path());
    QCOMPARE(vp->strokeColor(), QColor(Qt::white));
    QCOMPARE(vp->strokeWidth(), 1.0f);
    QCOMPARE(vp->fillColor(), QColor(Qt::white));
    QCOMPARE(vp->fillRule(), QQuickVisualPath::OddEvenFill);
    QCOMPARE(vp->joinStyle(), QQuickVisualPath::BevelJoin);
    QCOMPARE(vp->miterLimit(), 2);
    QCOMPARE(vp->capStyle(), QQuickVisualPath::SquareCap);
    QCOMPARE(vp->strokeStyle(), QQuickVisualPath::SolidLine);
    QCOMPARE(vp->dashOffset(), 0.0f);
    QCOMPARE(vp->dashPattern(), QVector<qreal>() << 4 << 2);
    QVERIFY(!vp->fillGradient());

    delete obj;
}

void tst_QQuickPathItem::basicPathItem()
{
    QScopedPointer<QQuickView> window(createView());

    window->setSource(testFileUrl("pathitem3.qml"));
    qApp->processEvents();

    QQuickPathItem *obj = findItem<QQuickPathItem>(window->rootObject(), "pathItem");
    QVERIFY(obj != nullptr);
    QQmlListReference list(obj, "elements");
    QCOMPARE(list.count(), 1);
    QQuickVisualPath *vp = qobject_cast<QQuickVisualPath *>(list.at(0));
    QVERIFY(vp != nullptr);
    QCOMPARE(vp->strokeWidth(), 4.0f);
    QVERIFY(vp->fillGradient() != nullptr);
    QVERIFY(vp->path() != nullptr);
    QCOMPARE(vp->strokeStyle(), QQuickVisualPath::DashLine);

    vp->setStrokeWidth(5.0f);
    QCOMPARE(vp->strokeWidth(), 5.0f);

    QQuickPathLinearGradient *lgrad = qobject_cast<QQuickPathLinearGradient *>(vp->fillGradient());
    QVERIFY(lgrad != nullptr);
    QCOMPARE(lgrad->spread(), QQuickPathGradient::PadSpread);
    QCOMPARE(lgrad->x1(), 20.0f);
    QQmlListReference stopList(lgrad, "stops");
    QCOMPARE(stopList.count(), 5);
    QVERIFY(stopList.at(2) != nullptr);

    QQuickPath *path = vp->path();
    QCOMPARE(path->startX(), 20.0f);
    QQmlListReference pathList(path, "pathElements");
    QCOMPARE(pathList.count(), 3);
}

void tst_QQuickPathItem::changeSignals()
{
    QScopedPointer<QQuickView> window(createView());

    window->setSource(testFileUrl("pathitem3.qml"));
    qApp->processEvents();

    QQuickPathItem *obj = findItem<QQuickPathItem>(window->rootObject(), "pathItem");
    QVERIFY(obj != nullptr);

    QSignalSpy asyncPropSpy(obj, SIGNAL(asynchronousChanged()));
    obj->setAsynchronous(true);
    obj->setAsynchronous(false);
    QCOMPARE(asyncPropSpy.count(), 2);

    QQmlListReference list(obj, "elements");
    QQuickVisualPath *vp = qobject_cast<QQuickVisualPath *>(list.at(0));
    QVERIFY(vp != nullptr);

    // Verify that VisualPath property changes emit changed().
    QSignalSpy vpChangeSpy(vp, SIGNAL(changed()));
    QSignalSpy strokeColorPropSpy(vp, SIGNAL(strokeColorChanged()));
    vp->setStrokeColor(Qt::blue);
    vp->setStrokeWidth(1.0f);
    QQuickPathGradient *g = vp->fillGradient();
    vp->setFillGradient(nullptr);
    vp->setFillColor(Qt::yellow);
    vp->setFillRule(QQuickVisualPath::WindingFill);
    vp->setJoinStyle(QQuickVisualPath::MiterJoin);
    vp->setMiterLimit(5);
    vp->setCapStyle(QQuickVisualPath::RoundCap);
    vp->setDashOffset(10);
    vp->setDashPattern(QVector<qreal>() << 1 << 2 << 3 << 4);
    QCOMPARE(strokeColorPropSpy.count(), 1);
    QCOMPARE(vpChangeSpy.count(), 10);

    // Verify that property changes from Path and its elements bubble up and result in changed().
    QQuickPath *path = vp->path();
    path->setStartX(30);
    QCOMPARE(vpChangeSpy.count(), 11);
    QQmlListReference pathList(path, "pathElements");
    qobject_cast<QQuickPathLine *>(pathList.at(1))->setY(200);
    QCOMPARE(vpChangeSpy.count(), 12);

    // Verify that property changes from the gradient bubble up and result in changed().
    vp->setFillGradient(g);
    QCOMPARE(vpChangeSpy.count(), 13);
    QQuickPathLinearGradient *lgrad = qobject_cast<QQuickPathLinearGradient *>(g);
    lgrad->setX2(200);
    QCOMPARE(vpChangeSpy.count(), 14);
    QQmlListReference stopList(lgrad, "stops");
    QCOMPARE(stopList.count(), 5);
    qobject_cast<QQuickPathGradientStop *>(stopList.at(1))->setPosition(0.3);
    QCOMPARE(vpChangeSpy.count(), 15);
    qobject_cast<QQuickPathGradientStop *>(stopList.at(1))->setColor(Qt::black);
    QCOMPARE(vpChangeSpy.count(), 16);
}

void tst_QQuickPathItem::render()
{
    QScopedPointer<QQuickView> window(createView());

    window->setSource(testFileUrl("pathitem3.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QImage img = window->grabWindow();
    QVERIFY(!img.isNull());

    QImage refImg(testFileUrl("pathitem3.png").toLocalFile());
    QVERIFY(!refImg.isNull());

    QVERIFY(QQuickVisualTestUtil::compareImages(img.convertToFormat(refImg.format()), refImg));
}

void tst_QQuickPathItem::renderWithMultipleVp()
{
    QScopedPointer<QQuickView> window(createView());

    window->setSource(testFileUrl("pathitem4.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QImage img = window->grabWindow();
    QVERIFY(!img.isNull());

    QImage refImg(testFileUrl("pathitem4.png").toLocalFile());
    QVERIFY(!refImg.isNull());

    QVERIFY(QQuickVisualTestUtil::compareImages(img.convertToFormat(refImg.format()), refImg));
}

QTEST_MAIN(tst_QQuickPathItem)

#include "tst_qquickpathitem.moc"
