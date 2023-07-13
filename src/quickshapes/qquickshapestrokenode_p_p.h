// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSHAPESTROKENODE_P_P_H
#define QQUICKSHAPESTROKENODE_P_P_H

#include <QtQuick/qsgmaterial.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of a number of Qt sources files.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QQuickShapeStrokeNode;
class QQuickShapeStrokeMaterial;

class QQuickShapeStrokeMaterialShader : public QSGMaterialShader
{
public:
    QQuickShapeStrokeMaterialShader()
    {
        setShaderFileName(VertexStage,
                          QStringLiteral(":/qt-project.org/shapes/shaders_ng/shapestroke.vert.qsb"));
        setShaderFileName(FragmentStage,
                          QStringLiteral(":/qt-project.org/shapes/shaders_ng/shapestroke.frag.qsb"));
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
};


class QQuickShapeStrokeMaterial : public QSGMaterial
{
public:
    QQuickShapeStrokeMaterial(QQuickShapeStrokeNode *node)
        : m_node(node)
    {
        setFlag(Blending, true);
    }

    int compare(const QSGMaterial *other) const override;

    QQuickShapeStrokeNode *node() const
    {
        return m_node;
    }

protected:
    QSGMaterialType *type() const override
    {
        static QSGMaterialType t;
        return &t;
    }
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override
    {
        return new QQuickShapeStrokeMaterialShader;
    }

    QQuickShapeStrokeNode *m_node;
};

QT_END_NAMESPACE

#endif // QQUICKSHAPESTROKENODE_P_P_H
