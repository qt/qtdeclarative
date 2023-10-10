// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgcurvestrokenode_p_p.h"
#include "qsgcurvestrokenode_p.h"

QT_BEGIN_NAMESPACE

bool QSGCurveStrokeMaterialShader::updateUniformData(RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    bool changed = false;
    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 64);

    auto *newMaterial = static_cast<QSGCurveStrokeMaterial *>(newEffect);
    auto *oldMaterial = static_cast<QSGCurveStrokeMaterial *>(oldEffect);

    auto *newNode = newMaterial != nullptr ? newMaterial->node() : nullptr;
    auto *oldNode = oldMaterial != nullptr ? oldMaterial->node() : nullptr;

    if (state.isMatrixDirty()) {
        QMatrix4x4 m = state.combinedMatrix();
        float localScale = newNode != nullptr ? newNode->localScale() : 1.0f;
        m.scale(localScale);
        memcpy(buf->data(), m.constData(), 64);

        float matrixScale = qSqrt(qAbs(state.determinant())) * state.devicePixelRatio() * localScale;
        memcpy(buf->data()+64, &matrixScale, 4);
        changed = true;
    }

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 64 + 4, &opacity, 4);
        changed = true;
    }

    int offset = 64+16;
    if (newNode == nullptr)
        return changed;

    QVector4D newStrokeColor(newNode->color().redF(),
                             newNode->color().greenF(),
                             newNode->color().blueF(),
                             newNode->color().alphaF());
    QVector4D oldStrokeColor = oldNode != nullptr
                                   ? QVector4D(oldNode->color().redF(),
                                               oldNode->color().greenF(),
                                               oldNode->color().blueF(),
                                               oldNode->color().alphaF())
                                   : QVector4D{};

    if (oldNode == nullptr || oldStrokeColor != newStrokeColor) {
        memcpy(buf->data() + offset, &newStrokeColor, 16);
        changed = true;
    }
    offset += 16;

    if (oldNode == nullptr || newNode->strokeWidth() != oldNode->strokeWidth()) {
        float w = newNode->strokeWidth();
        memcpy(buf->data() + offset, &w, 4);
        changed = true;
    }
    offset += 4;
    if (oldNode == nullptr || newNode->debug() != oldNode->debug()) {
        float w = newNode->debug();
        memcpy(buf->data() + offset, &w, 4);
        changed = true;
    }
//    offset += 4;

    return changed;
}

int QSGCurveStrokeMaterial::compare(const QSGMaterial *other) const
{
    int typeDif = type() - other->type();
    if (!typeDif) {
        auto *othernode = static_cast<const QSGCurveStrokeMaterial*>(other)->node();
        if (node()->color() != othernode->color())
            return node()->color().rgb() < othernode->color().rgb() ? -1 : 1;
        if (node()->strokeWidth() != othernode->strokeWidth())
            return node()->strokeWidth() < othernode->strokeWidth() ? -1 : 1;
    }
    return typeDif;
}

QT_END_NAMESPACE
