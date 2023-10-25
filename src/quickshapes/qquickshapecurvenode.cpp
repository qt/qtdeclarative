// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickshapecurvenode_p.h"
#include "qquickshapecurvenode_p_p.h"

QT_BEGIN_NAMESPACE

namespace {
}

QQuickShapeCurveNode::QQuickShapeCurveNode()
{
    setFlag(OwnsGeometry, true);
    setGeometry(new QSGGeometry(attributes(), 0, 0));

    updateMaterial();
}

void QQuickShapeCurveNode::updateMaterial()
{
    m_material.reset(new QQuickShapeCurveMaterial(this));
    setMaterial(m_material.data());
}

void QQuickShapeCurveNode::cookGeometry()
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

const QSGGeometry::AttributeSet &QQuickShapeCurveNode::attributes()
{
    static QSGGeometry::Attribute data[] = {
        QSGGeometry::Attribute::createWithAttributeType(0, 2, QSGGeometry::FloatType, QSGGeometry::PositionAttribute),
        QSGGeometry::Attribute::createWithAttributeType(1, 3, QSGGeometry::FloatType, QSGGeometry::TexCoordAttribute),
        QSGGeometry::Attribute::createWithAttributeType(2, 4, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute),
        QSGGeometry::Attribute::createWithAttributeType(3, 2, QSGGeometry::FloatType, QSGGeometry::UnknownAttribute),
    };
    static QSGGeometry::AttributeSet attrs = { 4, sizeof(CurveNodeVertex), data };
    return attrs;
}

QT_END_NAMESPACE
