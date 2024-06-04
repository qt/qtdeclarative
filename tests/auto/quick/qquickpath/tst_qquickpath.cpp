// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/private/qquickpath_p.h>
#include <QtQuick/private/qquickrectangle_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_QuickPath : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QuickPath() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void arc();
    void angleArc();
    void catmullRomCurve();
    void closedCatmullRomCurve();
    void svg();
    void line();
    void rectangle_data();
    void rectangle();
    void rectangleRadii();
    void appendRemove();

private:
    void arc(QSizeF scale);
    void angleArc(QSizeF scale);
    void catmullRomCurve(QSizeF scale, const QVector<QPointF> &points);
    void closedCatmullRomCurve(QSizeF scale, const QVector<QPointF> &points);
    void svg(QSizeF scale);
    void line(QSizeF scale);
    void rectangle(const QQuickPath *path, const QRectF &rect);
};

static void compare(const QPointF &point, const QSizeF &scale, int line, double x, double y)
{
    QVERIFY2(qFuzzyCompare(float(point.x()), float(x * scale.width())),
             (QStringLiteral("Actual: ") + QString::number(point.x(),'g',14)
              + QStringLiteral(" Expected: ") + QString::number(x * scale.width(),'g',14)
              + QStringLiteral(" At: ") + QString::number(line)).toLatin1().data());
    QVERIFY2(qFuzzyCompare(float(point.y()), float(y * scale.height())),
             (QStringLiteral("Actual: ") + QString::number(point.y(),'g',14)
              + QStringLiteral(" Expected: ") + QString::number(y * scale.height(),'g',14)
              + QStringLiteral(" At: ") + QString::number(line)).toLatin1().data());
}
static void compare(const QPointF &point, int line, const QPointF &pt)
{
    return compare(point, QSizeF(1,1), line, pt.x(), pt.y());
}

void tst_QuickPath::arc(QSizeF scale)
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("arc.qml"));
    QQuickPath *obj = qobject_cast<QQuickPath*>(c.create());
    QVERIFY(obj != nullptr);
    if (scale != QSizeF(1,1))
        obj->setProperty("scale", scale);

    QCOMPARE(obj->startX(), 0.);
    QCOMPARE(obj->startY(), 0.);

    QQmlListReference list(obj, "pathElements");
    QCOMPARE(list.count(), 1);

    QQuickPathArc* arc = qobject_cast<QQuickPathArc*>(list.at(0));
    QVERIFY(arc != nullptr);
    QCOMPARE(arc->x(), 100.);
    QCOMPARE(arc->y(), 100.);
    QCOMPARE(arc->radiusX(), 100.);
    QCOMPARE(arc->radiusY(), 100.);
    QCOMPARE(arc->useLargeArc(), false);
    QCOMPARE(arc->direction(), QQuickPathArc::Clockwise);

    QPainterPath path = obj->path();
    QVERIFY(path != QPainterPath());

    QPointF pos = obj->pointAtPercent(0);
    QCOMPARE(pos, QPointF(0,0));
    pos = obj->pointAtPercent(.25);
    compare(pos, scale, __LINE__, 38.9244897744, 7.85853964341);
    pos = obj->pointAtPercent(.75);
    compare(pos, scale, __LINE__, 92.141460356592, 61.07551022559);
    pos = obj->pointAtPercent(1);
    QCOMPARE(pos, QPointF(100 * scale.width(), 100 * scale.height()));
}

void tst_QuickPath::arc()
{
    arc(QSizeF(1,1));
    arc(QSizeF(2.2,3.4));
}

void tst_QuickPath::angleArc(QSizeF scale)
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("anglearc.qml"));
    QQuickPath *obj = qobject_cast<QQuickPath*>(c.create());
    QVERIFY(obj != nullptr);
    if (scale != QSizeF(1,1))
        obj->setProperty("scale", scale);

    QQmlListReference list(obj, "pathElements");
    QCOMPARE(list.count(), 1);

    QQuickPathAngleArc* arc = qobject_cast<QQuickPathAngleArc*>(list.at(0));
    QVERIFY(arc != nullptr);
    QCOMPARE(arc->centerX(), 100.);
    QCOMPARE(arc->centerY(), 100.);
    QCOMPARE(arc->radiusX(), 50.);
    QCOMPARE(arc->radiusY(), 50.);
    QCOMPARE(arc->startAngle(), 45.);
    QCOMPARE(arc->sweepAngle(), 90.);
    QCOMPARE(arc->moveToStart(), true);

    QPainterPath path = obj->path();
    QVERIFY(path != QPainterPath());

    QPointF pos = obj->pointAtPercent(0);
    compare(pos, scale, __LINE__, 135.35533905867, 135.35533905867);
    pos = obj->pointAtPercent(.25);
    compare(pos, scale, __LINE__, 119.46222180396, 146.07068621369);
    pos = obj->pointAtPercent(.75);
    compare(pos, scale, __LINE__, 80.537778196007, 146.07068621366);
    pos = obj->pointAtPercent(1);
    compare(pos, scale, __LINE__, 64.644660941173, 135.35533905867);

    // if moveToStart is false, we should have a line starting from startX/Y
    arc->setMoveToStart(false);
    pos = obj->pointAtPercent(0);
    QCOMPARE(pos, QPointF(0,0));
}

void tst_QuickPath::angleArc()
{
    angleArc(QSizeF(1,1));
    angleArc(QSizeF(2.7,0.92));
}

void tst_QuickPath::catmullRomCurve(QSizeF scale, const QVector<QPointF> &points)
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("curve.qml"));
    QQuickPath *obj = qobject_cast<QQuickPath*>(c.create());
    QVERIFY(obj != nullptr);
    if (scale != QSizeF(1,1))
        obj->setProperty("scale", scale);

    QCOMPARE(obj->startX(), 0.);
    QCOMPARE(obj->startY(), 0.);

    QQmlListReference list(obj, "pathElements");
    QCOMPARE(list.count(), 3);

    QQuickPathCatmullRomCurve* curve = qobject_cast<QQuickPathCatmullRomCurve*>(list.at(0));
    QVERIFY(curve != nullptr);
    QCOMPARE(curve->x(), 100.);
    QCOMPARE(curve->y(), 50.);

    curve = qobject_cast<QQuickPathCatmullRomCurve*>(list.at(2));
    QVERIFY(curve != nullptr);
    QCOMPARE(curve->x(), 100.);
    QCOMPARE(curve->y(), 150.);

    QPainterPath path = obj->path();
    QVERIFY(path != QPainterPath());

    QPointF pos = path.pointAtPercent(0);
    QCOMPARE(pos, points.at(0));
    pos = path.pointAtPercent(.25);
    compare(pos,  __LINE__, points.at(1));
    pos = path.pointAtPercent(.75);
    compare(pos,  __LINE__, points.at(2));
    pos = path.pointAtPercent(1);
    compare(pos,  __LINE__, points.at(3));
}

void tst_QuickPath::catmullRomCurve()
{
    catmullRomCurve(QSizeF(1,1), { QPointF(0,0),
                                  QPointF(62.917022919131, 26.175485291549),
                                  QPointF(51.194527196674 , 105.27985623074),
                                  QPointF(100, 150) });
    catmullRomCurve(QSizeF(2,5.3), { QPointF(0,0),
                                    QPointF(150.80562419914, 170.34065984615),
                                    QPointF(109.08400252853 , 588.35165918579),
                                    QPointF(200, 795) });
}

void tst_QuickPath::closedCatmullRomCurve(QSizeF scale, const QVector<QPointF> &points)
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("closedcurve.qml"));
    QQuickPath *obj = qobject_cast<QQuickPath*>(c.create());
    QVERIFY(obj != nullptr);
    if (scale != QSizeF(1,1))
        obj->setProperty("scale", scale);

    QCOMPARE(obj->startX(), 50.);
    QCOMPARE(obj->startY(), 50.);

    QQmlListReference list(obj, "pathElements");
    QCOMPARE(list.count(), 3);

    QQuickPathCatmullRomCurve* curve = qobject_cast<QQuickPathCatmullRomCurve*>(list.at(2));
    QVERIFY(curve != nullptr);
    QCOMPARE(curve->x(), 50.);
    QCOMPARE(curve->y(), 50.);

    QVERIFY(obj->isClosed());

    QPainterPath path = obj->path();
    QVERIFY(path != QPainterPath());

    QPointF pos = path.pointAtPercent(0);
    QCOMPARE(pos, points.at(0));
    pos = path.pointAtPercent(.1);
    compare(pos,  __LINE__, points.at(1));
    pos = path.pointAtPercent(.75);
    compare(pos,  __LINE__, points.at(2));
    pos = path.pointAtPercent(1);
    compare(pos,  __LINE__, points.at(3));
}

void tst_QuickPath::closedCatmullRomCurve()
{
    closedCatmullRomCurve(QSizeF(1,1), { QPointF(50,50),
                                         QPointF(66.776225481812, 55.617435304145),
                                         QPointF(44.10269379731 , 116.33512508175),
                                         QPointF(50, 50) });
    closedCatmullRomCurve(QSizeF(2,3), { QPointF(100,150),
                                         QPointF(136.49725836178, 170.25466686363),
                                         QPointF(87.713232151943 , 328.29232737977),
                                         QPointF(100, 150) });
}

void tst_QuickPath::svg(QSizeF scale)
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("svg.qml"));
    QQuickPath *obj = qobject_cast<QQuickPath*>(c.create());
    QVERIFY(obj != nullptr);
    if (scale != QSizeF(1,1))
        obj->setProperty("scale", scale);

    QCOMPARE(obj->startX(), 0.);
    QCOMPARE(obj->startY(), 0.);

    QQmlListReference list(obj, "pathElements");
    QCOMPARE(list.count(), 1);

    QQuickPathSvg* svg = qobject_cast<QQuickPathSvg*>(list.at(0));
    QVERIFY(svg != nullptr);
    QCOMPARE(svg->path(), QLatin1String("M200,300 Q400,50 600,300 T1000,300"));

    QPainterPath path = obj->path();
    QVERIFY(path != QPainterPath());

    QPointF pos = obj->pointAtPercent(0);
    QCOMPARE(pos, QPointF(200 * scale.width(),300 * scale.height()));
    pos = obj->pointAtPercent(.25);
    QCOMPARE(pos.toPoint(), QPoint(400 * scale.width(),175 * scale.height()));  //fuzzy compare
    pos = obj->pointAtPercent(.75);
    QCOMPARE(pos.toPoint(), QPoint(800 * scale.width(),425 * scale.height())); //fuzzy compare
    pos = obj->pointAtPercent(1);
    QCOMPARE(pos, QPointF(1000 * scale.width(),300 * scale.height()));
}

void tst_QuickPath::svg()
{
    svg(QSizeF(1,1));
    svg(QSizeF(5,3));
}

void tst_QuickPath::line(QSizeF scale)
{
    QQmlEngine engine;
    QQmlComponent c1(&engine);
    c1.setData(
            "import QtQuick 2.0\n"
            "Path {\n"
                "startX: 0; startY: 0\n"
                "PathLine { x: 100; y: 100 }\n"
            "}", QUrl());
    QScopedPointer<QObject> o1(c1.create());
    QQuickPath *path1 = qobject_cast<QQuickPath *>(o1.data());
    QVERIFY(path1);
    if (scale != QSizeF(1,1))
        path1->setProperty("scale", scale);

    QQmlComponent c2(&engine);
    c2.setData(
            "import QtQuick 2.0\n"
            "Path {\n"
                "startX: 0; startY: 0\n"
                "PathLine { x: 50; y: 50 }\n"
                "PathLine { x: 100; y: 100 }\n"
            "}", QUrl());
    QScopedPointer<QObject> o2(c2.create());
    QQuickPath *path2 = qobject_cast<QQuickPath *>(o2.data());
    QVERIFY(path2);
    if (scale != QSizeF(1,1))
        path2->setProperty("scale", scale);

    for (int i = 0; i < 167; ++i) {
        qreal t = i / 167.0;

        QPointF p1 = path1->pointAtPercent(t);
        QCOMPARE(p1.x(), p1.y());

        QPointF p2 = path2->pointAtPercent(t);
        QCOMPARE(p1.toPoint(), p2.toPoint());
    }
}

void tst_QuickPath::line()
{
    line(QSizeF(1,1));
    line(QSizeF(7.23,7.23));
}

void tst_QuickPath::rectangle_data()
{
    QTest::addColumn<QByteArray>("pathqml");
    QTest::addColumn<QRectF>("rect");

    QTest::newRow("basic") << QByteArray("PathRectangle { width: 100; height: 100 }\n")
                           << QRectF(0, 0, 100, 100);

    QTest::newRow("relative") << QByteArray("startX: -50; startY: -100\nPathRectangle {"
                                            "relativeX: 100.2; relativeY: 200.3;"
                                            "width: 10.5; height: 10.5 }\n")
                              << QRectF(50.2, 100.3, 10.5, 10.5);

    QTest::newRow("stroke") << QByteArray("PathRectangle { x: 5; y: 10; width: 100; height: 100;"
                                          "strokeAdjustment: 20 }\n")
                            << QRectF(5, 10, 100, 100).adjusted(10, 10, -10, -10);
}

void tst_QuickPath::rectangle(const QQuickPath *path, const QRectF &rect)
{
    QCOMPARE(path->pointAtPercent(0), rect.topLeft());
    QCOMPARE(path->pointAtPercent(1), rect.topLeft());
    QCOMPARE(path->pointAtPercent(1.0 / 8), QPointF(rect.center().x(), rect.top()));
    QCOMPARE(path->pointAtPercent(3.0 / 8), QPointF(rect.right(), rect.center().y()));
    QCOMPARE(path->pointAtPercent(5.0 / 8), QPointF(rect.center().x(), rect.bottom()));
    QCOMPARE(path->pointAtPercent(7.0 / 8), QPointF(rect.left(), rect.center().y()));
}

void tst_QuickPath::rectangle()
{
    QFETCH(QByteArray, pathqml);
    QFETCH(QRectF, rect);

    QQmlEngine engine;
    QQmlComponent c1(&engine);
    c1.setData("import QtQuick\nPath {\n" + pathqml + "}", QUrl());
    QScopedPointer<QObject> o1(c1.create());
    QQuickPath *path = qobject_cast<QQuickPath *>(o1.data());
    QVERIFY(path);
    QCOMPARE(path->pointAtPercent(0), rect.topLeft());
    QCOMPARE(path->pointAtPercent(1), rect.topLeft());
    QCOMPARE(path->pointAtPercent(1.0 / 8), QPointF(rect.center().x(), rect.top()));
    QCOMPARE(path->pointAtPercent(3.0 / 8), QPointF(rect.right(), rect.center().y()));
    QCOMPARE(path->pointAtPercent(5.0 / 8), QPointF(rect.center().x(), rect.bottom()));
    QCOMPARE(path->pointAtPercent(7.0 / 8), QPointF(rect.left(), rect.center().y()));
}

#define COMPARE_RADII(P, Q)                                                                        \
    QCOMPARE(P.radius(), Q->radius());                                                             \
    QCOMPARE(P.topLeftRadius(), Q->topLeftRadius());                                               \
    QCOMPARE(P.topRightRadius(), Q->topRightRadius());                                             \
    QCOMPARE(P.bottomLeftRadius(), Q->bottomLeftRadius());                                         \
    QCOMPARE(P.bottomRightRadius(), Q->bottomRightRadius());

void tst_QuickPath::rectangleRadii()
{
    // Test that the radius logic of PathRectangle is the same as Rectangle's
    QQmlEngine engine;
    QQmlComponent c1(&engine);
    c1.setData("import QtQuick\n"
               "Rectangle { x: 10; y: 20; width: 30; height: 40\n"
               "}",
               QUrl());
    QScopedPointer<QObject> o1(c1.create());
    QQuickRectangle *quickRectangle = qobject_cast<QQuickRectangle *>(o1.data());
    QVERIFY(quickRectangle);
    QQuickPathRectangle pathRectangle;
    pathRectangle.setX(quickRectangle->x());
    pathRectangle.setY(quickRectangle->y());
    pathRectangle.setWidth(quickRectangle->width());
    pathRectangle.setHeight(quickRectangle->height());
    COMPARE_RADII(pathRectangle, quickRectangle);
    pathRectangle.setRadius(5);
    quickRectangle->setRadius(5);
    COMPARE_RADII(pathRectangle, quickRectangle);
    pathRectangle.setBottomLeftRadius(15);
    quickRectangle->setBottomLeftRadius(15);
    COMPARE_RADII(pathRectangle, quickRectangle);
    pathRectangle.setRadius(-5);
    quickRectangle->setRadius(-5);
    COMPARE_RADII(pathRectangle, quickRectangle);
    pathRectangle.setRadius(0);
    quickRectangle->setRadius(0);
    COMPARE_RADII(pathRectangle, quickRectangle);
    pathRectangle.setTopLeftRadius(-7);
    quickRectangle->setTopLeftRadius(-7);
    COMPARE_RADII(pathRectangle, quickRectangle);
    pathRectangle.setRadius(4);
    quickRectangle->setRadius(4);
    pathRectangle.resetBottomLeftRadius();
    quickRectangle->resetBottomLeftRadius();
    pathRectangle.setTopRightRadius(0);
    quickRectangle->setTopRightRadius(0);
    pathRectangle.setTopLeftRadius(200);
    quickRectangle->setTopLeftRadius(200);
    COMPARE_RADII(pathRectangle, quickRectangle);
}

void tst_QuickPath::appendRemove()
{
    QQuickPath *path = new QQuickPath();
    QQmlListReference pathElements(path, "pathElements");
    QSignalSpy changedSpy(path, SIGNAL(changed()));

    QCOMPARE(pathElements.count(), 0);
    QCOMPARE(changedSpy.count(), 0);

    pathElements.append(new QQuickPathElement(path));
    pathElements.append(new QQuickPathElement(path));
    pathElements.append(new QQuickPathElement(path));

    QCOMPARE(pathElements.count(), 3);
    QCOMPARE(changedSpy.count(), 3);

    pathElements.clear();

    QCOMPARE(pathElements.count(), 0);
    QCOMPARE(changedSpy.count(), 4);
}

QTEST_MAIN(tst_QuickPath)

#include "tst_qquickpath.moc"
