// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qsgcurvefillnode_p.h"
#include "qsgcurvefillnode_p_p.h"

QT_BEGIN_NAMESPACE

QSGCurveFillNode::QSGCurveFillNode()
{
    setFlag(OwnsGeometry, true);
    setFlag(UsePreprocess, true);
    setGeometry(new QSGGeometry(attributes(), 0, 0));

    updateMaterial();
}

void QSGCurveFillNode::updateMaterialFlags()
{
    Q_ASSERT(m_material != nullptr);

    const bool wasFullMatrixEnabled = m_material->flags().testFlag(QSGMaterial::RequiresFullMatrix);
    const bool needsFullMatrixEnabled =
        m_gradientType != QGradient::NoGradient || m_textureProvider != nullptr;

    if (wasFullMatrixEnabled == needsFullMatrixEnabled)
        return;

    m_material->setFlag(QSGMaterial::RequiresFullMatrix, needsFullMatrixEnabled);

    // Clearing RequiresFullMatrix also clears RequiresFullMatrixExceptTranslate, so we need
    // to restore it.
    if (!needsFullMatrixEnabled)
        m_material->setFlag(QSGMaterial::RequiresFullMatrixExceptTranslate, true);

    markDirty(DirtyGeometry);
}

void QSGCurveFillNode::updateMaterial()
{
    m_material.reset(new QSGCurveFillMaterial(this));
    setMaterial(m_material.data());
    updateMaterialFlags();
}

void QSGCurveFillNode::cookGeometry()
{
    // The batch renderer can only merge geometry with 16-bit indices, so use
    // those whenever the vertex count allows it.
    const QSGGeometry::Type indexType = m_uncookedVertexes.size() <= 0xffff
            ? QSGGeometry::UnsignedShortType
            : QSGGeometry::UnsignedIntType;
    QSGGeometry *g = geometry();
    if (g->indexType() != indexType) {
        g = new QSGGeometry(attributes(),
                            m_uncookedVertexes.size(),
                            m_uncookedIndexes.size(),
                            indexType);
        setGeometry(g);
    } else {
        g->allocate(m_uncookedVertexes.size(), m_uncookedIndexes.size());
    }

    g->setDrawingMode(QSGGeometry::DrawTriangles);
    memcpy(g->vertexData(),
           m_uncookedVertexes.constData(),
           g->vertexCount() * g->sizeOfVertex());
    if (indexType == QSGGeometry::UnsignedShortType) {
        quint16 *dst = static_cast<quint16 *>(g->indexData());
        for (qsizetype i = 0; i < m_uncookedIndexes.size(); ++i)
            dst[i] = quint16(m_uncookedIndexes.at(i));
    } else {
        memcpy(g->indexData(),
               m_uncookedIndexes.constData(),
               g->indexCount() * g->sizeOfIndex());
    }

    m_uncookedIndexes.clear();
    m_uncookedIndexes.squeeze();
    m_uncookedVertexes.clear();
    m_uncookedVertexes.squeeze();
}

const QSGGeometry::AttributeSet &QSGCurveFillNode::attributes()
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

#include "moc_qsgcurvefillnode_p.cpp"
