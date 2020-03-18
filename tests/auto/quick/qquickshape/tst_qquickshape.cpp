/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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
#include <QtQuickShapes/private/qquickshape_p.h>
#include <QStandardPaths>

#include "../../shared/util.h"
#include "../shared/viewtestutil.h"
#include "../shared/visualtestutil.h"

using namespace QQuickViewTestUtil;
using namespace QQuickVisualTestUtil;

class PolygonProvider : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVector<QPolygonF> paths READ paths WRITE setPaths NOTIFY pathsChanged)

public:
    QVector<QPolygonF> paths() const { return m_paths; }
    void setPaths(QVector<QPolygonF> paths)
    {
        if (m_paths == paths)
            return;
        m_paths = paths;
        emit pathsChanged();
    }

signals:
    void pathsChanged();

private:
    QVector<QPolygonF> m_paths;
};

class tst_QQuickShape : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickShape();

private slots:
    void initValues();
    void vpInitValues();
    void basicShape();
    void changeSignals();
    void render();
    void renderWithMultipleSp();
    void radialGrad();
    void conicalGrad();
    void renderPolyline();
    void renderMultiline();
    void polylineDataTypes_data();
    void polylineDataTypes();
    void multilineDataTypes_data();
    void multilineDataTypes();
    void multilineStronglyTyped();

private:
    QVector<QPolygonF> m_lowPolyLogo;
};

tst_QQuickShape::tst_QQuickShape()
{
    // Force the software backend to get reliable rendering results regardless of the hw and drivers.
    QQuickWindow::setSceneGraphBackend(QSGRendererInterface::Software);

    const char *uri = "tst_qquickpathitem";
    qmlRegisterType<QQuickShape>(uri, 1, 0, "Shape");
    qmlRegisterType<QQuickShapePath, 14>(uri, 1, 0, "ShapePath");
    qmlRegisterUncreatableType<QQuickShapeGradient>(uri, 1, 0, "ShapeGradient", QQuickShapeGradient::tr("ShapeGradient is an abstract base class"));
    qmlRegisterType<QQuickShapeLinearGradient>(uri, 1, 0, "LinearGradient");
    qmlRegisterType<QQuickShapeRadialGradient>(uri, 1, 0, "RadialGradient");
    qmlRegisterType<QQuickShapeConicalGradient>(uri, 1, 0, "ConicalGradient");
    qmlRegisterType<QQuickPathPolyline>(uri, 1, 0, "PathPolyline");
    qmlRegisterType<PolygonProvider>("Qt.test", 1, 0, "PolygonProvider");

    m_lowPolyLogo << (QPolygonF() <<
                      QPointF(20, 0) <<
                      QPointF(140, 0) <<
                      QPointF(140, 80) <<
                      QPointF(120, 100) <<
                      QPointF(0, 100) <<
                      QPointF(0, 20) <<
                      QPointF(20, 0) )
                  << (QPolygonF() << QPointF(20, 80) <<
                      QPointF(60, 80) <<
                      QPointF(80, 60) <<
                      QPointF(80, 20) <<
                      QPointF(40, 20) <<
                      QPointF(20, 40) <<
                      QPointF(20, 80) )
                  << (QPolygonF() << QPointF(80, 80) <<
                      QPointF(70, 70) )
                  << (QPolygonF() << QPointF(120, 80) <<
                      QPointF(100, 60) <<
                      QPointF(100, 20) )
                  << (QPolygonF() << QPointF(100, 40) <<
                      QPointF(120, 40) );
}

void tst_QQuickShape::initValues()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("pathitem1.qml"));
    QQuickShape *obj = qobject_cast<QQuickShape *>(c.create());

    QVERIFY(obj != nullptr);
    QVERIFY(obj->rendererType() == QQuickShape::UnknownRenderer);
    QVERIFY(!obj->asynchronous());
    QVERIFY(!obj->vendorExtensionsEnabled());
    QVERIFY(obj->status() == QQuickShape::Null);
    auto vps = obj->data();
    QVERIFY(vps.count(&vps) == 0);

    delete obj;
}

void tst_QQuickShape::vpInitValues()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("pathitem2.qml"));
    QQuickShape *obj = qobject_cast<QQuickShape *>(c.create());

    QVERIFY(obj != nullptr);
    QVERIFY(obj->rendererType() == QQuickShape::UnknownRenderer);
    QVERIFY(!obj->asynchronous());
    QVERIFY(!obj->vendorExtensionsEnabled());
    QVERIFY(obj->status() == QQuickShape::Null);
    auto vps = obj->data();
    QVERIFY(vps.count(&vps) == 2);

    QQuickShapePath *vp = qobject_cast<QQuickShapePath *>(vps.at(&vps, 0));
    QVERIFY(vp != nullptr);
    QQmlListReference pathList(vp, "pathElements");
    QCOMPARE(pathList.count(), 0);
    QCOMPARE(vp->strokeColor(), QColor(Qt::white));
    QCOMPARE(vp->strokeWidth(), 1.0f);
    QCOMPARE(vp->fillColor(), QColor(Qt::white));
    QCOMPARE(vp->fillRule(), QQuickShapePath::OddEvenFill);
    QCOMPARE(vp->joinStyle(), QQuickShapePath::BevelJoin);
    QCOMPARE(vp->miterLimit(), 2);
    QCOMPARE(vp->capStyle(), QQuickShapePath::SquareCap);
    QCOMPARE(vp->strokeStyle(), QQuickShapePath::SolidLine);
    QCOMPARE(vp->dashOffset(), 0.0f);
    QCOMPARE(vp->dashPattern(), QVector<qreal>() << 4 << 2);
    QVERIFY(!vp->fillGradient());

    delete obj;
}

void tst_QQuickShape::basicShape()
{
    QScopedPointer<QQuickView> window(createView());

    window->setSource(testFileUrl("pathitem3.qml"));
    qApp->processEvents();

    QQuickShape *obj = findItem<QQuickShape>(window->rootObject(), "pathItem");
    QVERIFY(obj != nullptr);
    QQmlListReference list(obj, "data");
    QCOMPARE(list.count(), 1);
    QQuickShapePath *vp = qobject_cast<QQuickShapePath *>(list.at(0));
    QVERIFY(vp != nullptr);
    QCOMPARE(vp->strokeWidth(), 4.0f);
    QVERIFY(vp->fillGradient() != nullptr);
    QCOMPARE(vp->strokeStyle(), QQuickShapePath::DashLine);

    vp->setStrokeWidth(5.0f);
    QCOMPARE(vp->strokeWidth(), 5.0f);

    QQuickShapeLinearGradient *lgrad = qobject_cast<QQuickShapeLinearGradient *>(vp->fillGradient());
    QVERIFY(lgrad != nullptr);
    QCOMPARE(lgrad->spread(), QQuickShapeGradient::PadSpread);
    QCOMPARE(lgrad->x1(), 20.0f);
    QQmlListReference stopList(lgrad, "stops");
    QCOMPARE(stopList.count(), 5);
    QVERIFY(stopList.at(2) != nullptr);

    QQuickPath *path = vp;
    QCOMPARE(path->startX(), 20.0f);
    QQmlListReference pathList(path, "pathElements");
    QCOMPARE(pathList.count(), 3);
}

void tst_QQuickShape::changeSignals()
{
    QScopedPointer<QQuickView> window(createView());

    window->setSource(testFileUrl("pathitem3.qml"));
    qApp->processEvents();

    QQuickShape *obj = findItem<QQuickShape>(window->rootObject(), "pathItem");
    QVERIFY(obj != nullptr);

    QSignalSpy asyncPropSpy(obj, SIGNAL(asynchronousChanged()));
    obj->setAsynchronous(true);
    obj->setAsynchronous(false);
    QCOMPARE(asyncPropSpy.count(), 2);

    QQmlListReference list(obj, "data");
    QQuickShapePath *vp = qobject_cast<QQuickShapePath *>(list.at(0));
    QVERIFY(vp != nullptr);

    // Verify that VisualPath property changes emit shapePathChanged().
    QSignalSpy vpChangeSpy(vp, SIGNAL(shapePathChanged()));
    QSignalSpy strokeColorPropSpy(vp, SIGNAL(strokeColorChanged()));
    vp->setStrokeColor(Qt::blue);
    vp->setStrokeWidth(1.0f);
    QQuickShapeGradient *g = vp->fillGradient();
    vp->setFillGradient(nullptr);
    vp->setFillColor(Qt::yellow);
    vp->setFillRule(QQuickShapePath::WindingFill);
    vp->setJoinStyle(QQuickShapePath::MiterJoin);
    vp->setMiterLimit(5);
    vp->setCapStyle(QQuickShapePath::RoundCap);
    vp->setDashOffset(10);
    vp->setDashPattern(QVector<qreal>() << 1 << 2 << 3 << 4);
    QCOMPARE(strokeColorPropSpy.count(), 1);
    QCOMPARE(vpChangeSpy.count(), 10);

    // Verify that property changes from Path and its elements bubble up and result in shapePathChanged().
    QQuickPath *path = vp;
    path->setStartX(30);
    QCOMPARE(vpChangeSpy.count(), 11);
    QQmlListReference pathList(path, "pathElements");
    qobject_cast<QQuickPathLine *>(pathList.at(1))->setY(200);
    QCOMPARE(vpChangeSpy.count(), 12);

    // Verify that property changes from the gradient bubble up and result in shapePathChanged().
    vp->setFillGradient(g);
    QCOMPARE(vpChangeSpy.count(), 13);
    QQuickShapeLinearGradient *lgrad = qobject_cast<QQuickShapeLinearGradient *>(g);
    lgrad->setX2(200);
    QCOMPARE(vpChangeSpy.count(), 14);
    QQmlListReference stopList(lgrad, "stops");
    QCOMPARE(stopList.count(), 5);
    qobject_cast<QQuickGradientStop *>(stopList.at(1))->setPosition(0.3);
    QCOMPARE(vpChangeSpy.count(), 15);
    qobject_cast<QQuickGradientStop *>(stopList.at(1))->setColor(Qt::black);
    QCOMPARE(vpChangeSpy.count(), 16);
}

void tst_QQuickShape::render()
{
    QScopedPointer<QQuickView> window(createView());

    window->setSource(testFileUrl("pathitem3.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QEXPECT_FAIL("", "Failure due to grabWindow not functional on offscreen/minimal platforms", Abort);

    QImage img = window->grabWindow();
    QVERIFY(!img.isNull());

    QImage refImg(testFileUrl("pathitem3.png").toLocalFile());
    QVERIFY(!refImg.isNull());

    QString errorMessage;
    const QImage actualImg = img.convertToFormat(refImg.format());
    QVERIFY2(QQuickVisualTestUtil::compareImages(actualImg, refImg, &errorMessage),
             qPrintable(errorMessage));
}

void tst_QQuickShape::renderWithMultipleSp()
{
    QScopedPointer<QQuickView> window(createView());

    window->setSource(testFileUrl("pathitem4.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QEXPECT_FAIL("", "Failure due to grabWindow not functional on offscreen/minimal platforms", Abort);

    QImage img = window->grabWindow();
    QVERIFY(!img.isNull());

    QImage refImg(testFileUrl("pathitem4.png").toLocalFile());
    QVERIFY(!refImg.isNull());

    QString errorMessage;
    const QImage actualImg = img.convertToFormat(refImg.format());
    QVERIFY2(QQuickVisualTestUtil::compareImages(actualImg, refImg, &errorMessage),
             qPrintable(errorMessage));
}

void tst_QQuickShape::radialGrad()
{
    QScopedPointer<QQuickView> window(createView());

    window->setSource(testFileUrl("pathitem5.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QEXPECT_FAIL("", "Failure due to grabWindow not functional on offscreen/minimal platforms", Abort);

    QImage img = window->grabWindow();
    QVERIFY(!img.isNull());

    QImage refImg(testFileUrl("pathitem5.png").toLocalFile());
    QVERIFY(!refImg.isNull());

    QString errorMessage;
    const QImage actualImg = img.convertToFormat(refImg.format());
    QVERIFY2(QQuickVisualTestUtil::compareImages(actualImg, refImg, &errorMessage),
             qPrintable(errorMessage));
}

void tst_QQuickShape::conicalGrad()
{
    QScopedPointer<QQuickView> window(createView());

    window->setSource(testFileUrl("pathitem6.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QEXPECT_FAIL("", "Failure due to grabWindow not functional on offscreen/minimal platforms", Abort);

    QImage img = window->grabWindow();
    QVERIFY(!img.isNull());

    QImage refImg(testFileUrl("pathitem6.png").toLocalFile());
    QVERIFY(!refImg.isNull());

    QString errorMessage;
    const QImage actualImg = img.convertToFormat(refImg.format());
    QVERIFY2(QQuickVisualTestUtil::compareImages(actualImg, refImg, &errorMessage),
             qPrintable(errorMessage));
}

void tst_QQuickShape::renderPolyline()
{
    QScopedPointer<QQuickView> window(createView());

    window->setSource(testFileUrl("pathitem7.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QEXPECT_FAIL("", "Failure due to grabWindow not functional on offscreen/minimal platforms", Abort);

    QImage img = window->grabWindow();
    QVERIFY(!img.isNull());

    QImage refImg(testFileUrl("pathitem3.png").toLocalFile()); // It's a recreation of pathitem3 using PathPolyline
    QVERIFY(!refImg.isNull());

    QString errorMessage;
    const QImage actualImg = img.convertToFormat(refImg.format());
    const bool res = QQuickVisualTestUtil::compareImages(actualImg, refImg, &errorMessage);
    if (!res) { // For visual inspection purposes.
        QTest::qWait(5000);
        const QString &tempLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        actualImg.save(tempLocation + QLatin1String("/pathitem7.png"));
    }
    QVERIFY2(res, qPrintable(errorMessage));
}

void tst_QQuickShape::renderMultiline()
{
    QScopedPointer<QQuickView> window(createView());

    window->setSource(testFileUrl("pathitem8.qml"));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QEXPECT_FAIL("", "Failure due to grabWindow not functional on offscreen/minimal platforms", Abort);

    QImage img = window->grabWindow();
    QVERIFY(!img.isNull());

    QImage refImg(testFileUrl("pathitem8.png").toLocalFile());
    QVERIFY(!refImg.isNull());

    QString errorMessage;
    const QImage actualImg = img.convertToFormat(refImg.format());
    const bool res = QQuickVisualTestUtil::compareImages(actualImg, refImg, &errorMessage);
    if (!res) { // For visual inspection purposes.
        QTest::qWait(5000);
        const QString &tempLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        actualImg.save(tempLocation + QLatin1String("/pathitem8.png"));
    }
    QVERIFY2(res, qPrintable(errorMessage));
}

void tst_QQuickShape::polylineDataTypes_data()
{
    QTest::addColumn<QVariant>("path");

    QTest::newRow("polygon") << QVariant::fromValue(m_lowPolyLogo.first());
    {
        QVector<QPointF> points;
        points << m_lowPolyLogo.first();
        QTest::newRow("vector of points") << QVariant::fromValue(points);
    }
    {
        QList<QPointF> points;
        for (const auto &point : m_lowPolyLogo.first())
            points << point;
        QTest::newRow("list of points") << QVariant::fromValue(points);
    }
    {
        QVariantList points;
        for (const auto &point : m_lowPolyLogo.first())
            points << point;
        QTest::newRow("QVariantList of points") << QVariant::fromValue(points);
    }
    {
        QVector<QPoint> points;
        for (const auto &point : m_lowPolyLogo.first())
            points << point.toPoint();
        QTest::newRow("vector of QPoint (integer points)") << QVariant::fromValue(points);
    }
    // Oddly, QPolygon is not supported, even though it's really QVector<QPoint>.
    // We don't want to have a special case for it in QQuickPathPolyline::setPath(),
    // but it could potentially be supported by fixing one of the QVariant conversions.
}

void tst_QQuickShape::polylineDataTypes()
{
    QFETCH(QVariant, path);

    QScopedPointer<QQuickView> window(createView());
    window->setSource(testFileUrl("polyline.qml"));
    QQuickShape *shape = qobject_cast<QQuickShape *>(window->rootObject());
    QVERIFY(shape);
    shape->setProperty("path", path);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QEXPECT_FAIL("", "Failure due to grabWindow not functional on offscreen/minimal platforms", Abort);

    QImage img = window->grabWindow();
    QVERIFY(!img.isNull());

    QImage refImg(testFileUrl("polyline.png").toLocalFile());
    QVERIFY(!refImg.isNull());

    QString errorMessage;
    const QImage actualImg = img.convertToFormat(refImg.format());
    const bool res = QQuickVisualTestUtil::compareImages(actualImg, refImg, &errorMessage);
    if (!res) { // For visual inspection purposes.
        QTest::qWait(5000);
        const QString &tempLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        actualImg.save(tempLocation + QLatin1String("/polyline.png"));
    }
    QVERIFY2(res, qPrintable(errorMessage));

    QCOMPARE(shape->property("path").value<QVector<QPointF>>(), m_lowPolyLogo.first());
    // Verify that QML sees it as an array of points
    int i = 0;
    for (QPointF p : m_lowPolyLogo.first()) {
        QMetaObject::invokeMethod(shape, "checkVertexAt", Q_ARG(QVariant, QVariant::fromValue<int>(i++)));
        QCOMPARE(shape->property("vertexBeingChecked").toPointF(), p);
    }
}

void tst_QQuickShape::multilineDataTypes_data()
{
    QTest::addColumn<QVariant>("paths");

    QTest::newRow("vector of polygons") << QVariant::fromValue(m_lowPolyLogo);
    {
        QVector<QVector<QPointF>> paths;
        for (const auto &poly : m_lowPolyLogo) {
            QVector<QPointF> points;
            points << poly;
            paths << points;
        }
        QTest::newRow("vector of point vectors") << QVariant::fromValue(paths);
    }
    {
        QList<QVector<QPointF>> paths;
        for (const auto &poly : m_lowPolyLogo) {
            QVector<QPointF> points;
            points << poly;
            paths << points;
        }
        QTest::newRow("list of point vectors") << QVariant::fromValue(paths);
    }
    {
        QList<QList<QPointF>> paths;
        for (const auto &poly : m_lowPolyLogo) {
            QList<QPointF> points;
            for (const auto &point : poly)
                points << point;
            paths << points;
        }
        QTest::newRow("list of point lists") << QVariant::fromValue(paths);
    }
    {
        QVariantList paths;
        for (const auto &poly : m_lowPolyLogo) {
            QVector<QPointF> points;
            points << poly;
            paths << QVariant::fromValue(points);
        }
        QTest::newRow("QVariantList of point vectors") << QVariant::fromValue(paths);
    }
    {
        QVariantList paths;
        for (const auto &poly : m_lowPolyLogo) {
            QList<QPointF> points;
            for (const auto &point : poly)
                points << point;
            paths << QVariant::fromValue(points);
        }
        QTest::newRow("QVariantList of point lists") << QVariant::fromValue(paths);
    }
    {
        QVariantList paths;
        for (const auto &poly : m_lowPolyLogo) {
            QVariantList points;
            for (const auto &point : poly)
                points << point;
            paths << QVariant::fromValue(points);
        }
        QTest::newRow("QVariantList of QVariantLists") << QVariant::fromValue(paths);
    }
    /* These could be supported if QVariant knew how to convert lists and vectors of QPolygon to QPolygonF.
       But they are omitted for now because we don't want to have special cases for them
       in QQuickPathMultiline::setPaths().  Floating point is preferred for geometry in Qt Quick.
    {
        QList<QPolygon> paths;
        for (const auto &poly : m_lowPolyLogo)
            paths << poly.toPolygon();
        QTest::newRow("list of QPolygon (integer points)") << QVariant::fromValue(paths);
    }
    {
        QVector<QPolygon> paths;
        for (const auto &poly : m_lowPolyLogo)
            paths << poly.toPolygon();
        QTest::newRow("vector of QPolygon (integer points)") << QVariant::fromValue(paths);
    }
    */
    {
        QList<QList<QPoint>> paths;
        for (const auto &poly : m_lowPolyLogo) {
            QList<QPoint> points;
            for (const auto &point : poly)
                points << point.toPoint();
            paths << points;
        }
        QTest::newRow("list of integer point lists") << QVariant::fromValue(paths);
    }
    {
        QVector<QList<QPoint>> paths;
        for (const auto &poly : m_lowPolyLogo) {
            QList<QPoint> points;
            for (const auto &point : poly)
                points << point.toPoint();
            paths << points;
        }
        QTest::newRow("vector of integer point lists") << QVariant::fromValue(paths);
    }
    {
        QList<QVector<QPoint>> paths;
        for (const auto &poly : m_lowPolyLogo) {
            QVector<QPoint> points;
            for (const auto &point : poly)
                points << point.toPoint();
            paths << points;
        }
        QTest::newRow("list of integer point vectors") << QVariant::fromValue(paths);
    }
}

void tst_QQuickShape::multilineDataTypes()
{
    QFETCH(QVariant, paths);

    QScopedPointer<QQuickView> window(createView());
    window->setSource(testFileUrl("multiline.qml"));
    QQuickShape *shape = qobject_cast<QQuickShape *>(window->rootObject());
    QVERIFY(shape);
    shape->setProperty("paths", paths);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QEXPECT_FAIL("", "Failure due to grabWindow not functional on offscreen/minimal platforms", Abort);

    QImage img = window->grabWindow();
    QVERIFY(!img.isNull());

    QImage refImg(testFileUrl("multiline.png").toLocalFile());
    QVERIFY(!refImg.isNull());

    QString errorMessage;
    const QImage actualImg = img.convertToFormat(refImg.format());
    const bool res = QQuickVisualTestUtil::compareImages(actualImg, refImg, &errorMessage);
    if (!res) { // For visual inspection purposes.
        QTest::qWait(5000);
        const QString &tempLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        actualImg.save(tempLocation + QLatin1String("/multiline.png"));
    }
    QVERIFY2(res, qPrintable(errorMessage));

    QVector<QVector<QPointF>> pointVectors;
    for (auto v : m_lowPolyLogo)
        pointVectors << v;
    QCOMPARE(shape->property("paths").value<QVector<QVector<QPointF>>>(), pointVectors);
    // Verify that QML sees it as an array of arrays of points
    int i = 0;
    for (auto pv : m_lowPolyLogo) {
        int j = 0;
        for (QPointF p : pv) {
            QMetaObject::invokeMethod(shape, "checkVertexAt", Q_ARG(QVariant, QVariant::fromValue<int>(i)), Q_ARG(QVariant, QVariant::fromValue<int>(j++)));
            QCOMPARE(shape->property("vertexBeingChecked").toPointF(), p);
        }
        ++i;
    }
}

void tst_QQuickShape::multilineStronglyTyped()
{
    QScopedPointer<QQuickView> window(createView());
    window->setSource(testFileUrl("multilineStronglyTyped.qml"));
    QQuickShape *shape = qobject_cast<QQuickShape *>(window->rootObject());
    QVERIFY(shape);
    PolygonProvider *provider = shape->findChild<PolygonProvider*>("provider");
    QVERIFY(provider);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
    provider->setPaths(m_lowPolyLogo);

    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
            || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QEXPECT_FAIL("", "Failure due to grabWindow not functional on offscreen/minimal platforms", Abort);

    QImage img = window->grabWindow();
    QVERIFY(!img.isNull());

    QImage refImg(testFileUrl("multiline.png").toLocalFile());
    QVERIFY(!refImg.isNull());

    QString errorMessage;
    const QImage actualImg = img.convertToFormat(refImg.format());
    const bool res = QQuickVisualTestUtil::compareImages(actualImg, refImg, &errorMessage);
    if (!res) { // For visual inspection purposes.
        QTest::qWait(5000);
        const QString &tempLocation = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        actualImg.save(tempLocation + QLatin1String("/multilineStronglyTyped.png"));
    }
    QVERIFY2(res, qPrintable(errorMessage));

    QVector<QVector<QPointF>> pointVectors;
    for (auto v : m_lowPolyLogo)
        pointVectors << v;
    QCOMPARE(shape->property("paths").value<QVector<QVector<QPointF>>>(), pointVectors);
    // Verify that QML sees it as an array of arrays of points
    int i = 0;
    for (auto pv : m_lowPolyLogo) {
        int j = 0;
        for (QPointF p : pv) {
            QMetaObject::invokeMethod(shape, "checkVertexAt", Q_ARG(QVariant, QVariant::fromValue<int>(i)), Q_ARG(QVariant, QVariant::fromValue<int>(j++)));
            QCOMPARE(shape->property("vertexBeingChecked").toPointF(), p);
        }
        ++i;
    }
}

QTEST_MAIN(tst_QQuickShape)

#include "tst_qquickshape.moc"
