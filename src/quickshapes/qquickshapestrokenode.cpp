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

// Find the parameters H, G for the depressed cubic
// t^2+H*t+G=0
// that results from the equation
// Q'(s).(p-Q(s)) = 0
// The last parameter is the static offset between s and t:
// s = t - b/(3a)
// use it to get back the parameter t
QVector3D QQuickShapeStrokeNode::HGforPoint(QVector2D q_a, QVector2D q_b, QVector2D q_c, QVector2D p)
{
    // this is a constant for the curve
    float a = -2. * QVector2D::dotProduct(q_a, q_a);
    // this is a constant for the curve
    float b = -3. * QVector2D::dotProduct(q_a, q_b);
    //this is linear in p so it can be put into the shader with vertex data
    float c = 2. * QVector2D::dotProduct(q_a, p) - QVector2D::dotProduct(q_b, q_b) - 2. * QVector2D::dotProduct(q_a, q_c);
    //this is linear in p so it can be put into the shader with vertex data
    float d = QVector2D::dotProduct(q_b,p) - QVector2D::dotProduct(q_b, q_c);
    // convert to depressed cubic.
    // both functions are linear in c and d and thus linear in p
    // Put in vertex data.
    float H = (3. * a * c - b * b) / (3. * a * a);
    float G = (2. * b * b * b - 9. * a * b * c + 27. * a * a * d) / (27. * a * a * a);

    return QVector3D(H, G, b/(3*a));
}

// Take the start, control and end point of a curve and return the points A, B, C
// representing the curve as Q(s) = A*s*s + B*s + C
std::array<QVector2D, 3> QQuickShapeStrokeNode::curveABC(QVector2D p0, QVector2D p1, QVector2D p2)
{
    QVector2D a = p0 - 2*p1 + p2;
    QVector2D b = 2*p1 - 2*p0;
    QVector2D c = p0;

    return {a, b, c};
}

void QQuickShapeStrokeNode::appendTriangle(const QVector2D &v0, const QVector2D &v1, const QVector2D &v2,
                    const QVector2D &p0, const QVector2D &p1, const QVector2D &p2)
{
    auto abc = curveABC(p0, p1, p2);

    int currentVertex = m_uncookedVertexes.count();

    for (auto p : QList<QVector2D>({v0, v1, v2})) {
        auto hg = HGforPoint(abc[0], abc[1], abc[2], p);

        m_uncookedVertexes.append( { p.x(), p.y(),
                               abc[0].x(), abc[0].y(), abc[1].x(), abc[1].y(), abc[2].x(), abc[2].y(),
                               hg.x(), hg.y(),
                               hg.z()} );
    }
    m_uncookedIndexes << currentVertex << currentVertex + 1 << currentVertex + 2;
}

void QQuickShapeStrokeNode::appendTriangle(const QVector2D &v0, const QVector2D &v1, const QVector2D &v2,
                    const QVector2D &p0, const QVector2D &p1)
{
    // We could reduce this to a linear equation by setting A to (0,0).
    // However, then we cannot use the cubic solution and need an additional
    // code path in the shader. The following formulation looks more complicated
    // but allows to always use the cubic solution.
    auto A = p1 - p0;
    auto B = QVector2D(0., 0.);
    auto C = p0;

    int currentVertex = m_uncookedVertexes.count();

    for (auto p : QList<QVector2D>({v0, v1, v2})) {
        auto hg = HGforPoint(A, B, C, p);
        m_uncookedVertexes.append( { p.x(), p.y(),
                               A.x(), A.y(), B.x(), B.y(), C.x(), C.y(),
                               hg.x(), hg.y(),
                               hg.z()} );
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
        QSGGeometry::Attribute::createWithAttributeType(4, 2, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute), //HG
        QSGGeometry::Attribute::createWithAttributeType(5, 1, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute), //offset

    };
    static QSGGeometry::AttributeSet attrs = { 6, sizeof(StrokeVertex), data };
    return attrs;
}

QT_END_NAMESPACE
