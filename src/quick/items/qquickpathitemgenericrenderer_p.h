/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKPATHITEMGENERICRENDERER_P_H
#define QQUICKPATHITEMGENERICRENDERER_P_H

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

#include "qquickpathitem_p_p.h"
#include <qsgnode.h>
#include <qsggeometry.h>
#include <qsgmaterial.h>
#include <QtGui/private/qtriangulatingstroker_p.h>

QT_BEGIN_NAMESPACE

class QQuickPathItemGenericRootRenderNode;

class QQuickPathItemGenericRenderer : public QQuickAbstractPathRenderer
{
public:
    enum Dirty {
        DirtyGeom = 0x01,
        DirtyColor = 0x02,
        DirtyFillGradient = 0x04,

        DirtyAll = 0xFF
    };

    QQuickPathItemGenericRenderer(QQuickItem *item)
        : m_item(item),
          m_rootNode(nullptr),
          m_effectiveDirty(0)
    { }

    void beginSync() override;
    void setPath(const QQuickPath *path) override;
    void setStrokeColor(const QColor &color) override;
    void setStrokeWidth(qreal w) override;
    void setFillColor(const QColor &color) override;
    void setFillRule(QQuickPathItem::FillRule fillRule) override;
    void setJoinStyle(QQuickPathItem::JoinStyle joinStyle, int miterLimit) override;
    void setCapStyle(QQuickPathItem::CapStyle capStyle) override;
    void setStrokeStyle(QQuickPathItem::StrokeStyle strokeStyle,
                        qreal dashOffset, const QVector<qreal> &dashPattern) override;
    void setFillGradient(QQuickPathGradient *gradient) override;
    void endSync() override;
    void updatePathRenderNode() override;

    void setRootNode(QQuickPathItemGenericRootRenderNode *rn);

    struct Color4ub { unsigned char r, g, b, a; };

private:
    void triangulateFill();
    void triangulateStroke();
    void updateFillNode();
    void updateStrokeNode();

    QQuickItem *m_item;
    QQuickPathItemGenericRootRenderNode *m_rootNode;
    QTriangulatingStroker m_stroker;
    QDashedStrokeProcessor m_dashStroker;

    QPen m_pen;
    Color4ub m_strokeColor;
    Color4ub m_fillColor;
    Qt::FillRule m_fillRule;
    QPainterPath m_path;
    bool m_fillGradientActive;
    QQuickPathItemGradientCache::GradientDesc m_fillGradient;

    QVector<QSGGeometry::ColoredPoint2D> m_fillVertices;
    QVector<quint16> m_fillIndices;
    QVector<QSGGeometry::ColoredPoint2D> m_strokeVertices;

    int m_syncDirty;
    int m_effectiveDirty;
};

class QQuickPathItemGenericRenderNode : public QSGGeometryNode
{
public:
    QQuickPathItemGenericRenderNode(QQuickWindow *window, QQuickPathItemGenericRootRenderNode *rootNode);
    ~QQuickPathItemGenericRenderNode();

    enum Material {
        MatSolidColor,
        MatLinearGradient
    };

    void activateMaterial(Material m);

    QQuickWindow *window() const { return m_window; }
    QQuickPathItemGenericRootRenderNode *rootNode() const { return m_rootNode; }

    // shadow data for custom materials
    QQuickPathItemGradientCache::GradientDesc m_fillGradient;

private:
    QSGGeometry m_geometry;
    QQuickWindow *m_window;
    QQuickPathItemGenericRootRenderNode *m_rootNode;
    QSGMaterial *m_material;
    QScopedPointer<QSGMaterial> m_solidColorMaterial;
    QScopedPointer<QSGMaterial> m_linearGradientMaterial;

    friend class QQuickPathItemGenericRenderer;
};

class QQuickPathItemGenericRootRenderNode : public QSGNode
{
public:
    QQuickPathItemGenericRootRenderNode(QQuickWindow *window, bool hasFill, bool hasStroke);
    ~QQuickPathItemGenericRootRenderNode();

private:
    QQuickPathItemGenericRenderNode *m_fillNode;
    QQuickPathItemGenericRenderNode *m_strokeNode;

    friend class QQuickPathItemGenericRenderer;
};

class QQuickPathItemGenericMaterialFactory
{
public:
    static QSGMaterial *createVertexColor(QQuickWindow *window);
    static QSGMaterial *createLinearGradient(QQuickWindow *window, QQuickPathItemGenericRenderNode *node);
};

#ifndef QT_NO_OPENGL

class QQuickPathItemLinearGradientShader : public QSGMaterialShader
{
public:
    QQuickPathItemLinearGradientShader();

    void initialize() override;
    void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
    char const *const *attributeNames() const override;

    static QSGMaterialType type;

private:
    int m_opacityLoc;
    int m_matrixLoc;
    int m_gradStartLoc;
    int m_gradEndLoc;
};

class QQuickPathItemLinearGradientMaterial : public QSGMaterial
{
public:
    QQuickPathItemLinearGradientMaterial(QQuickPathItemGenericRenderNode *node)
        : m_node(node)
    {
        // Passing RequiresFullMatrix is essential in order to prevent the
        // batch renderer from baking in simple, translate-only transforms into
        // the vertex data. The shader will rely on the fact that
        // vertexCoord.xy is the PathItem-space coordinate and so no modifications
        // are welcome.
        setFlag(Blending | RequiresFullMatrix);
    }

    QSGMaterialType *type() const override
    {
        return &QQuickPathItemLinearGradientShader::type;
    }

    int compare(const QSGMaterial *other) const override;

    QSGMaterialShader *createShader() const override
    {
        return new QQuickPathItemLinearGradientShader;
    }

    QQuickPathItemGenericRenderNode *node() const { return m_node; }

private:
    QQuickPathItemGenericRenderNode *m_node;
};

#endif // QT_NO_OPENGL

QT_END_NAMESPACE

#endif // QQUICKPATHITEMGENERICRENDERER_P_H
