// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickshapestrokenode_p.h"
#include "qquickshapestrokenode_p_p.h"

QT_BEGIN_NAMESPACE

QQuickShapeStrokeNode::QQuickShapeStrokeNode()
{
    setFlag(OwnsGeometry, true);
    setGeometry(new QSGGeometry(attributes(), 0, 0));
    updateMaterial();
}

void QQuickShapeStrokeNode::QQuickShapeStrokeNode::updateMaterial()
{
    m_material.reset(new QQuickShapeStrokeMaterial(this));
    setMaterial(m_material.data());
}

// Take the start, control and end point of a curve and return the points A, B, C
// representing the curve as Q(s) = A*s*s + B*s + C
std::array<QVector2D, 3> QQuickShapeStrokeNode::curveABC(const std::array<QVector2D, 3> &p)
{
    QVector2D a = p[0] - 2*p[1] + p[2];
    QVector2D b = 2*p[1] - 2*p[0];
    QVector2D c = p[0];

    return {a, b, c};
}

// Curve from p[0] to p[2] with control point p[1]
void QQuickShapeStrokeNode::appendTriangle(const std::array<QVector2D, 3> &v,
                                           const std::array<QVector2D, 3> &p,
                                           const std::array<QVector2D, 3> &n)
{
    auto abc = curveABC(p);

    int currentVertex = m_uncookedVertexes.count();

    for (int i = 0; i < 3; ++i) {
        m_uncookedVertexes.append( { v[i].x(), v[i].y(),
                                   abc[0].x(), abc[0].y(), abc[1].x(), abc[1].y(), abc[2].x(), abc[2].y(),
                                   n[i].x(), n[i].y() } );
    }
    m_uncookedIndexes << currentVertex << currentVertex + 1 << currentVertex + 2;
}

// Straight line from p0 to p1
void QQuickShapeStrokeNode::appendTriangle(const std::array<QVector2D, 3> &v,
                                           const std::array<QVector2D, 2> &p,
                                           const std::array<QVector2D, 3> &n)
{
    // We could reduce this to a linear equation by setting A to (0,0).
    // However, then we cannot use the cubic solution and need an additional
    // code path in the shader. The following formulation looks more complicated
    // but allows to always use the cubic solution.
    auto A = p[1] - p[0];
    auto B = QVector2D(0., 0.);
    auto C = p[0];

    int currentVertex = m_uncookedVertexes.count();

//    for (auto v : QList<QPair<QVector2D, QVector2D>>({{v0, n0}, {v1, n1}, {v2, n2}})) {
    for (int i = 0; i < 3; ++i) {
        m_uncookedVertexes.append( { v[i].x(), v[i].y(),
                                   A.x(), A.y(), B.x(), B.y(), C.x(), C.y(),
                                   n[i].x(), n[i].y() } );
    }
    m_uncookedIndexes << currentVertex << currentVertex + 1 << currentVertex + 2;
}

void QQuickShapeStrokeNode::cookGeometry()
{
    QSGGeometry *g = geometry();
    if (g->indexType() != QSGGeometry::UnsignedIntType) {
        g = new QSGGeometry(attributes(),
                            m_uncookedVertexes.size(),
                            m_uncookedIndexes.size(),
                            QSGGeometry::UnsignedIntType);
        setGeometry(g);
    } else {
        g->allocate(m_uncookedVertexes.size(), m_uncookedIndexes.size());
    }

    g->setDrawingMode(QSGGeometry::DrawTriangles);
    memcpy(g->vertexData(),
           m_uncookedVertexes.constData(),
           g->vertexCount() * g->sizeOfVertex());
    memcpy(g->indexData(),
           m_uncookedIndexes.constData(),
           g->indexCount() * g->sizeOfIndex());

    m_uncookedIndexes.clear();
    m_uncookedVertexes.clear();
}

const QSGGeometry::AttributeSet &QQuickShapeStrokeNode::attributes()
{
    static QSGGeometry::Attribute data[] = {
        QSGGeometry::Attribute::createWithAttributeType(0, 2, QSGGeometry::FloatType, QSGGeometry::PositionAttribute), //vertexCoord
        QSGGeometry::Attribute::createWithAttributeType(1, 2, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute), //A
        QSGGeometry::Attribute::createWithAttributeType(2, 2, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute), //B
        QSGGeometry::Attribute::createWithAttributeType(3, 2, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute), //C
        QSGGeometry::Attribute::createWithAttributeType(4, 2, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute), //normalVector
    };
    static QSGGeometry::AttributeSet attrs = { 5, sizeof(StrokeVertex), data };
    return attrs;
}

QT_END_NAMESPACE
