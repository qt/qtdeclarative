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

#ifndef QQUICKPATHITEMNVPRRENDERER_P_H
#define QQUICKPATHITEMNVPRRENDERER_P_H

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
#include <qsgrendernode.h>
#include <private/qquicknvprfunctions_p.h>
#include <QColor>
#include <QVector4D>
#include <QDebug>

#ifndef QT_NO_OPENGL

QT_BEGIN_NAMESPACE

class QQuickPathItemNvprRenderNode;
class QOpenGLFramebufferObject;
class QOpenGLBuffer;
class QOpenGLExtraFunctions;

class QQuickPathItemNvprRenderer : public QQuickAbstractPathRenderer
{
public:
    enum Dirty {
        DirtyPath = 0x01,
        DirtyStyle = 0x02,
        DirtyFillRule = 0x04,
        DirtyDash = 0x08,
        DirtyFillGradient = 0x10,

        DirtyAll = 0xFF
    };

    QQuickPathItemNvprRenderer(QQuickItem *item)
        : m_item(item)
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

    void setNode(QQuickPathItemNvprRenderNode *node);

    struct NvprPath {
        QVector<GLubyte> cmd;
        QVector<GLfloat> coord;
    };

private:
    void convertPath(const QQuickPath *path);

    QQuickItem *m_item;
    QQuickPathItemNvprRenderNode *m_node = nullptr;
    int m_dirty = 0;

    NvprPath m_path;
    qreal m_strokeWidth;
    QColor m_strokeColor;
    QColor m_fillColor;
    QQuickPathItem::JoinStyle m_joinStyle;
    int m_miterLimit;
    QQuickPathItem::CapStyle m_capStyle;
    QQuickPathItem::FillRule m_fillRule;
    bool m_dashActive;
    qreal m_dashOffset;
    QVector<qreal> m_dashPattern;
    bool m_fillGradientActive;
    QQuickPathItemGradientCache::GradientDesc m_fillGradient;
};

QDebug operator<<(QDebug debug, const QQuickPathItemNvprRenderer::NvprPath &path);

class QQuickNvprMaterialManager
{
public:
    enum Material {
        MatSolid,
        MatLinearGradient,

        NMaterials
    };

    struct MaterialDesc {
        GLuint ppl = 0;
        GLuint prg = 0;
        int uniLoc[4];
    };

    void create(QQuickNvprFunctions *nvpr);
    MaterialDesc *activateMaterial(Material m);
    void releaseResources();

private:
    QQuickNvprFunctions *m_nvpr;
    MaterialDesc m_materials[NMaterials];
};

class QQuickNvprBlitter
{
public:
    bool create();
    void destroy();
    bool isCreated() const { return m_program != nullptr; }
    void texturedQuad(GLuint textureId, const QSize &size,
                      const QMatrix4x4 &proj, const QMatrix4x4 &modelview,
                      float opacity);

private:
    QOpenGLShaderProgram *m_program = nullptr;
    QOpenGLBuffer *m_buffer = nullptr;
    int m_matrixLoc;
    int m_opacityLoc;
    QSize m_prevSize;
};

class QQuickPathItemNvprRenderNode : public QSGRenderNode
{
public:
    QQuickPathItemNvprRenderNode(QQuickPathItem *item);
    ~QQuickPathItemNvprRenderNode();

    void render(const RenderState *state) override;
    void releaseResources() override;
    StateFlags changedStates() const override;
    RenderingFlags flags() const override;
    QRectF rect() const override;

    static bool isSupported();

private:
    void updatePath();
    void renderStroke(int strokeStencilValue, int writeMask);
    void renderFill();
    void renderOffscreenFill();
    void setupStencilForCover(bool stencilClip, int sv);

    static bool nvprInited;
    static QQuickNvprFunctions nvpr;
    static QQuickNvprMaterialManager mtlmgr;

    QQuickPathItem *m_item;
    GLuint m_path = 0;
    int m_dirty = 0;

    QQuickPathItemNvprRenderer::NvprPath m_source;
    GLfloat m_strokeWidth;
    QVector4D m_strokeColor;
    QVector4D m_fillColor;
    GLenum m_joinStyle;
    GLint m_miterLimit;
    GLenum m_capStyle;
    GLenum m_fillRule;
    GLfloat m_dashOffset;
    QVector<GLfloat> m_dashPattern;
    bool m_fillGradientActive;
    QQuickPathItemGradientCache::GradientDesc m_fillGradient;
    QOpenGLFramebufferObject *m_fallbackFbo = nullptr;
    QQuickNvprBlitter m_fallbackBlitter;
    QOpenGLExtraFunctions *f = nullptr;

    friend class QQuickPathItemNvprRenderer;
};

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif // QQUICKPATHITEMNVPRRENDERER_P_H
