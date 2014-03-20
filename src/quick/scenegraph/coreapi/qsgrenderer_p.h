/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSGRENDERER_P_H
#define QSGRENDERER_P_H

#include <qset.h>
#include <qhash.h>

#include <qcolor.h>
#include <qopenglfunctions.h>
#include <qopenglshaderprogram.h>

#include "qsgnode.h"
#include "qsgmaterial.h"
#include <QtQuick/qsgtexture.h>

#include <QtQuick/private/qsgcontext_p.h>

QT_BEGIN_NAMESPACE

class QSGMaterialShader;
struct QSGMaterialType;
class QOpenGLFramebufferObject;
class TextureReference;
class QSGBindable;
class QSGNodeUpdater;

class Q_QUICK_PRIVATE_EXPORT QSGRenderer : public QObject, public QOpenGLFunctions
{
    Q_OBJECT
public:
    enum ClipTypeBit
    {
        NoClip = 0x00,
        ScissorClip = 0x01,
        StencilClip = 0x02
    };
    Q_DECLARE_FLAGS(ClipType, ClipTypeBit)

    enum ClearModeBit
    {
        ClearColorBuffer    = 0x0001,
        ClearDepthBuffer    = 0x0002,
        ClearStencilBuffer  = 0x0004
    };
    Q_DECLARE_FLAGS(ClearMode, ClearModeBit)

    QSGRenderer(QSGRenderContext *context);
    virtual ~QSGRenderer();

    void setRootNode(QSGRootNode *node);
    QSGRootNode *rootNode() const { return m_root_node; }

    void setDeviceRect(const QRect &rect) { m_device_rect = rect; }
    inline void setDeviceRect(const QSize &size) { setDeviceRect(QRect(QPoint(), size)); }
    QRect deviceRect() const { return m_device_rect; }

    void setViewportRect(const QRect &rect) { m_viewport_rect = rect; }
    inline void setViewportRect(const QSize &size) { setViewportRect(QRect(QPoint(), size)); }
    QRect viewportRect() const { return m_viewport_rect; }

    // Accessed by QSGMaterialShader::RenderState.
    QMatrix4x4 currentProjectionMatrix() const { return m_current_projection_matrix; }
    QMatrix4x4 currentModelViewMatrix() const { return m_current_model_view_matrix; }
    QMatrix4x4 currentCombinedMatrix() const { return m_current_projection_matrix * m_current_model_view_matrix; }
    qreal currentOpacity() const { return m_current_opacity; }
    qreal determinant() const { return m_current_determinant; }

    void setDevicePixelRatio(qreal ratio) { m_device_pixel_ratio = ratio; }
    qreal devicePixelRatio() const { return m_device_pixel_ratio; }

    void setProjectionMatrixToDeviceRect();
    virtual void setProjectionMatrixToRect(const QRectF &rect);
    void setProjectionMatrix(const QMatrix4x4 &matrix);
    QMatrix4x4 projectionMatrix() const { return m_projection_matrix; }
    bool isMirrored() const { return m_mirrored; }

    void setClearColor(const QColor &color);
    QColor clearColor() const { return m_clear_color; }

    QSGRenderContext *context() const { return m_context; }

    void renderScene();
    void renderScene(const QSGBindable &bindable);
    virtual void nodeChanged(QSGNode *node, QSGNode::DirtyState state);
    virtual void materialChanged(QSGGeometryNode *node, QSGMaterial *from, QSGMaterial *to);

    QSGNodeUpdater *nodeUpdater() const;
    void setNodeUpdater(QSGNodeUpdater *updater);

    inline QSGMaterialShader::RenderState state(QSGMaterialShader::RenderState::DirtyStates dirty) const;

    void setClearMode(ClearMode mode) { m_clear_mode = mode; }
    ClearMode clearMode() const { return m_clear_mode; }

    virtual void setCustomRenderMode(const QByteArray &) { };

    void clearChangedFlag() { m_changed_emitted = false; }

Q_SIGNALS:
    void sceneGraphChanged(); // Add, remove, ChangeFlags changes...

protected:
    void draw(const QSGMaterialShader *material, const QSGGeometry *g);

    virtual void render() = 0;
    QSGRenderer::ClipType updateStencilClip(const QSGClipNode *clip);

    const QSGBindable *bindable() const { return m_bindable; }

    virtual void preprocess();

    void addNodesToPreprocess(QSGNode *node);
    void removeNodesToPreprocess(QSGNode *node);

    void markNodeDirtyState(QSGNode *node, QSGNode::DirtyState state) { node->m_dirtyState |= state; }

    QColor m_clear_color;
    ClearMode m_clear_mode;
    QMatrix4x4 m_current_projection_matrix;
    QMatrix4x4 m_current_model_view_matrix;
    qreal m_current_opacity;
    qreal m_current_determinant;
    qreal m_device_pixel_ratio;
    QRect m_current_scissor_rect;
    int m_current_stencil_value;

    QSGRenderContext *m_context;

private:
    QSGRootNode *m_root_node;
    QSGNodeUpdater *m_node_updater;

    QRect m_device_rect;
    QRect m_viewport_rect;

    QSet<QSGNode *> m_nodes_to_preprocess;

    QMatrix4x4 m_projection_matrix;
    QOpenGLShaderProgram m_clip_program;
    int m_clip_matrix_id;

    const QSGBindable *m_bindable;

    uint m_changed_emitted : 1;
    uint m_mirrored : 1;
    uint m_is_rendering : 1;

    uint m_vertex_buffer_bound : 1;
    uint m_index_buffer_bound : 1;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSGRenderer::ClearMode)

class Q_QUICK_PRIVATE_EXPORT QSGBindable
{
public:
    virtual ~QSGBindable() { }
    virtual void bind() const = 0;
    virtual void clear(QSGRenderer::ClearMode mode) const;
    virtual void reactivate() const;
};

class QSGBindableFbo : public QSGBindable
{
public:
    QSGBindableFbo(QOpenGLFramebufferObject *fbo);
    virtual void bind() const;
private:
    QOpenGLFramebufferObject *m_fbo;
};

class QSGBindableFboId : public QSGBindable
{
public:
    QSGBindableFboId(GLuint);
    virtual void bind() const;
private:
    GLuint m_id;
};



QSGMaterialShader::RenderState QSGRenderer::state(QSGMaterialShader::RenderState::DirtyStates dirty) const
{
    QSGMaterialShader::RenderState s;
    s.m_dirty = dirty;
    s.m_data = this;
    return s;
}


class Q_QUICK_PRIVATE_EXPORT QSGNodeDumper : public QSGNodeVisitor {

public:
    static void dump(QSGNode *n);

    QSGNodeDumper() : m_indent(0) {}
    void visitNode(QSGNode *n);
    void visitChildren(QSGNode *n);

private:
    int m_indent;
};



QT_END_NAMESPACE

#endif
