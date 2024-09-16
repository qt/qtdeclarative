// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QPainterPath>
#include <QSGNode>
#include <private/qquickshapecurverenderer_p.h>

class tst_CurveRenderer : public QObject
{
    Q_OBJECT

public:
    tst_CurveRenderer();

private slots:
    void initTestCase_data();

    void render_data();
    void render();
};

tst_CurveRenderer::tst_CurveRenderer()
{
}

void tst_CurveRenderer::initTestCase_data()
{
    QTest::addColumn<bool>("hasFill");
    QTest::addColumn<int>("strokeWidth");
    // tbd: dashed, gradient fill etc.

    QTest::newRow("onlyfill") << true << 0;
    QTest::newRow("onlystroke") << false << 10;
    QTest::newRow("strokeandfill") << true << 10;
}

void tst_CurveRenderer::render_data()
{
    QTest::addColumn<QPainterPath>("path");

    {
        QPainterPath path;
        path.moveTo(100, 400);
        path.lineTo(200, 400);
        path.quadTo(220, 500, 700, 700);
        path.cubicTo(600, 600, 800, 200, 200, 50);
        path.lineTo(50, 750);
        path.cubicTo(600, 700, 300, 200, 750, 50);
        path.cubicTo(800, 200, 300, 800, 300, 400);
        QTest::newRow("figure1") << path;
    }
}

void tst_CurveRenderer::render()
{
    QFETCH_GLOBAL(bool, hasFill);
    QFETCH_GLOBAL(int, strokeWidth);
    QFETCH(QPainterPath, path);

    QSGNode dummyNode;
    QQuickShapeCurveRenderer renderer(nullptr);
    renderer.setRootNode(&dummyNode);
    renderer.beginSync(1, nullptr); // Just sets the number of path items to 1; needs no endsync
    renderer.setFillColor(0, hasFill ? Qt::yellow : Qt::transparent);
    renderer.setStrokeColor(0, Qt::black);
    renderer.setStrokeWidth(0, strokeWidth);

    QBENCHMARK {
        renderer.beginSync(1, nullptr);
        renderer.setPath(0, path);
        renderer.endSync(false);
        renderer.updateNode();
    }
}

QTEST_MAIN(tst_CurveRenderer)
#include "tst_bench_curverenderer.moc"
