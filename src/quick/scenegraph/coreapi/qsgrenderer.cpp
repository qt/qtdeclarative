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

#include "qsgrenderer_p.h"
#include "qsgnode.h"
#include "qsgmaterial.h"
#include "qsgnodeupdater_p.h"
#include "qsggeometry_p.h"

#include <private/qsgadaptationlayer_p.h>
#include <private/qsgshadersourcebuilder_p.h>

#include <QOpenGLShaderProgram>
#include <qopenglframebufferobject.h>
#include <QtGui/qguiapplication.h>

#include <qdatetime.h>

#include <private/qquickprofiler_p.h>

QT_BEGIN_NAMESPACE

//#define RENDERER_DEBUG
//#define QT_GL_NO_SCISSOR_TEST

static bool qsg_sanity_check = qgetenv("QSG_SANITY_CHECK").toInt();

#ifndef QSG_NO_RENDER_TIMING
static bool qsg_render_timing = !qgetenv("QSG_RENDER_TIMING").isEmpty();
static QElapsedTimer frameTimer;
static qint64 preprocessTime;
static qint64 updatePassTime;
#endif

void QSGBindable::clear(QSGRenderer::ClearMode mode) const
{
    GLuint bits = 0;
    if (mode & QSGRenderer::ClearColorBuffer) bits |= GL_COLOR_BUFFER_BIT;
    if (mode & QSGRenderer::ClearDepthBuffer) bits |= GL_DEPTH_BUFFER_BIT;
    if (mode & QSGRenderer::ClearStencilBuffer) bits |= GL_STENCIL_BUFFER_BIT;
    glClear(bits);
}

// Reactivate the color buffer after switching to the stencil.
void QSGBindable::reactivate() const
{
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

QSGBindableFbo::QSGBindableFbo(QOpenGLFramebufferObject *fbo) : m_fbo(fbo)
{
}


void QSGBindableFbo::bind() const
{
    m_fbo->bind();
}

QSGBindableFboId::QSGBindableFboId(GLuint id)
    : m_id(id)
{
}


void QSGBindableFboId::bind() const
{
    QOpenGLContext *context = QOpenGLContext::currentContext();
    context->functions()->glBindFramebuffer(GL_FRAMEBUFFER, m_id);
    QOpenGLContextPrivate::get(context)->current_fbo = m_id;
}

/*!
    \class QSGRenderer
    \brief The renderer class is the abstract baseclass use for rendering the
    QML scene graph.

    The renderer is not tied to any particular surface. It expects a context to
    be current and will render into that surface according to how the device rect,
    viewport rect and projection transformation are set up.

    Rendering is a sequence of steps initiated by calling renderScene(). This will
    effectively draw the scene graph starting at the root node. The QSGNode::preprocess()
    function will be called for all the nodes in the graph, followed by an update
    pass which updates all matrices, opacity, clip states and similar in the graph.
    Because the update pass is called after preprocess, it is safe to modify the graph
    during preprocess. To run a custom update pass over the graph, install a custom
    QSGNodeUpdater using setNodeUpdater(). Once all the graphs dirty states are updated,
    the virtual render() function is called.

    The render() function is implemented by QSGRenderer subclasses to render the graph
    in the most optimal way for a given hardware.

    The renderer can make use of stencil, depth and color buffers in addition to the
    scissor rect.

    \internal
 */


QSGRenderer::QSGRenderer(QSGRenderContext *context)
    : QObject()
    , m_clear_color(Qt::transparent)
    , m_clear_mode(ClearColorBuffer | ClearDepthBuffer)
    , m_current_opacity(1)
    , m_current_determinant(1)
    , m_device_pixel_ratio(1)
    , m_current_stencil_value(0)
    , m_context(context)
    , m_root_node(0)
    , m_node_updater(0)
    , m_bindable(0)
    , m_changed_emitted(false)
    , m_mirrored(false)
    , m_is_rendering(false)
    , m_vertex_buffer_bound(false)
    , m_index_buffer_bound(false)
{
    initializeOpenGLFunctions();
}


QSGRenderer::~QSGRenderer()
{
    setRootNode(0);
    delete m_node_updater;
}

/*!
    Returns the node updater that this renderer uses to update states in the
    scene graph.

    If no updater is specified a default one is constructed.
 */

QSGNodeUpdater *QSGRenderer::nodeUpdater() const
{
    if (!m_node_updater)
        const_cast<QSGRenderer *>(this)->m_node_updater = new QSGNodeUpdater();
    return m_node_updater;
}


/*!
    Sets the node updater that this renderer uses to update states in the
    scene graph.

    This will delete and override any existing node updater
  */
void QSGRenderer::setNodeUpdater(QSGNodeUpdater *updater)
{
    if (m_node_updater)
        delete m_node_updater;
    m_node_updater = updater;
}


void QSGRenderer::setRootNode(QSGRootNode *node)
{
    if (m_root_node == node)
        return;
    if (m_root_node) {
        m_root_node->m_renderers.removeOne(this);
        nodeChanged(m_root_node, QSGNode::DirtyNodeRemoved);
    }
    m_root_node = node;
    if (m_root_node) {
        Q_ASSERT(!m_root_node->m_renderers.contains(this));
        m_root_node->m_renderers << this;
        nodeChanged(m_root_node, QSGNode::DirtyNodeAdded);
    }
}


void QSGRenderer::renderScene()
{
    class B : public QSGBindable
    {
    public:
        void bind() const { QOpenGLFramebufferObject::bindDefault(); }
    } b;
    renderScene(b);
}

void QSGRenderer::renderScene(const QSGBindable &bindable)
{
    if (!m_root_node)
        return;

    m_is_rendering = true;


#ifndef QSG_NO_RENDER_TIMING
    bool profileFrames = qsg_render_timing || QQuickProfiler::enabled;
    if (profileFrames)
        frameTimer.start();
    qint64 bindTime = 0;
    qint64 renderTime = 0;
#endif

    m_bindable = &bindable;
    preprocess();

    bindable.bind();
#ifndef QSG_NO_RENDER_TIMING
    if (profileFrames)
        bindTime = frameTimer.nsecsElapsed();
#endif

    // Sanity check that attribute registers are disabled
    if (qsg_sanity_check) {
        GLint count = 0;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &count);
        GLint enabled;
        for (int i=0; i<count; ++i) {
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
            if (enabled) {
                qWarning("QSGRenderer: attribute %d is enabled, this can lead to memory corruption and crashes.", i);
            }
        }
    }

    render();
#ifndef QSG_NO_RENDER_TIMING
    if (profileFrames)
        renderTime = frameTimer.nsecsElapsed();
#endif

    glDisable(GL_SCISSOR_TEST);
    m_is_rendering = false;
    m_changed_emitted = false;
    m_bindable = 0;

    if (m_vertex_buffer_bound) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_vertex_buffer_bound = false;
    }

    if (m_index_buffer_bound) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        m_index_buffer_bound = false;
    }

#ifndef QSG_NO_RENDER_TIMING
    if (qsg_render_timing) {
        qDebug(" - Breakdown of render time: preprocess=%d, updates=%d, binding=%d, render=%d, total=%d",
               int(preprocessTime / 1000000),
               int((updatePassTime - preprocessTime) / 1000000),
               int((bindTime - updatePassTime) / 1000000),
               int((renderTime - bindTime) / 1000000),
               int(renderTime / 1000000));
    }

    Q_QUICK_SG_PROFILE1(QQuickProfiler::SceneGraphRendererFrame, (
            preprocessTime,
            updatePassTime - preprocessTime,
            bindTime - updatePassTime,
            renderTime - bindTime));

#endif
}

void QSGRenderer::setProjectionMatrixToDeviceRect()
{
    setProjectionMatrixToRect(m_device_rect);
}

void QSGRenderer::setProjectionMatrixToRect(const QRectF &rect)
{
    QMatrix4x4 matrix;
    matrix.ortho(rect.x(),
                 rect.x() + rect.width(),
                 rect.y() + rect.height(),
                 rect.y(),
                 1,
                 -1);
    setProjectionMatrix(matrix);
}

void QSGRenderer::setProjectionMatrix(const QMatrix4x4 &matrix)
{
    m_projection_matrix = matrix;
    // Mirrored relative to the usual Qt coordinate system with origin in the top left corner.
    m_mirrored = matrix(0, 0) * matrix(1, 1) - matrix(0, 1) * matrix(1, 0) > 0;
}

void QSGRenderer::setClearColor(const QColor &color)
{
    m_clear_color = color;
}

/*!
    Updates internal data structures and emits the sceneGraphChanged() signal.

    If \a flags contains the QSGNode::DirtyNodeRemoved flag, the node might be
    in the process of being destroyed. It is then not safe to downcast the node
    pointer.
*/

void QSGRenderer::nodeChanged(QSGNode *node, QSGNode::DirtyState state)
{
    if (state & QSGNode::DirtyNodeAdded)
        addNodesToPreprocess(node);
    if (state & QSGNode::DirtyNodeRemoved)
        removeNodesToPreprocess(node);
    if (state & QSGNode::DirtyUsePreprocess) {
        if (node->flags() & QSGNode::UsePreprocess)
            m_nodes_to_preprocess.insert(node);
        else
            m_nodes_to_preprocess.remove(node);
    }

    if (!m_changed_emitted && !m_is_rendering) {
        // Premature overoptimization to avoid excessive signal emissions
        m_changed_emitted = true;
        emit sceneGraphChanged();
    }
}

void QSGRenderer::materialChanged(QSGGeometryNode *, QSGMaterial *, QSGMaterial *)
{
}

void QSGRenderer::preprocess()
{
    Q_ASSERT(m_root_node);

    // We need to take a copy here, in case any of the preprocess calls deletes a node that
    // is in the preprocess list and thus, changes the m_nodes_to_preprocess behind our backs
    // For the default case, when this does not happen, the cost is neglishible.
    QSet<QSGNode *> items = m_nodes_to_preprocess;

    for (QSet<QSGNode *>::const_iterator it = items.constBegin();
         it != items.constEnd(); ++it) {
        QSGNode *n = *it;
        if (!nodeUpdater()->isNodeBlocked(n, m_root_node)) {
            n->preprocess();
        }
    }

#ifndef QSG_NO_RENDER_TIMING
    bool profileFrames = qsg_render_timing || QQuickProfiler::enabled;
    if (profileFrames)
        preprocessTime = frameTimer.nsecsElapsed();
#endif

    nodeUpdater()->updateStates(m_root_node);

#ifndef QSG_NO_RENDER_TIMING
    if (profileFrames)
        updatePassTime = frameTimer.nsecsElapsed();
#endif

}

void QSGRenderer::addNodesToPreprocess(QSGNode *node)
{
    for (QSGNode *c = node->firstChild(); c; c = c->nextSibling())
        addNodesToPreprocess(c);
    if (node->flags() & QSGNode::UsePreprocess)
        m_nodes_to_preprocess.insert(node);
}

void QSGRenderer::removeNodesToPreprocess(QSGNode *node)
{
    for (QSGNode *c = node->firstChild(); c; c = c->nextSibling())
        removeNodesToPreprocess(c);
    if (node->flags() & QSGNode::UsePreprocess)
        m_nodes_to_preprocess.remove(node);
}


/*!
    Convenience function to set up the stencil buffer for clipping based on \a clip.

    If the clip is a pixel aligned rectangle, this function will use glScissor instead
    of stencil.
 */

QSGRenderer::ClipType QSGRenderer::updateStencilClip(const QSGClipNode *clip)
{
    if (!clip) {
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_SCISSOR_TEST);
        return NoClip;
    }

    ClipType clipType = NoClip;

    glDisable(GL_SCISSOR_TEST);

    m_current_stencil_value = 0;
    m_current_scissor_rect = QRect();
    while (clip) {
        QMatrix4x4 m = m_current_projection_matrix;
        if (clip->matrix())
            m *= *clip->matrix();

        // TODO: Check for multisampling and pixel grid alignment.
        bool isRectangleWithNoPerspective = clip->isRectangular()
                && qFuzzyIsNull(m(3, 0)) && qFuzzyIsNull(m(3, 1));
        bool noRotate = qFuzzyIsNull(m(0, 1)) && qFuzzyIsNull(m(1, 0));
        bool isRotate90 = qFuzzyIsNull(m(0, 0)) && qFuzzyIsNull(m(1, 1));

        if (isRectangleWithNoPerspective && (noRotate || isRotate90)) {
            QRectF bbox = clip->clipRect();
            qreal invW = 1 / m(3, 3);
            qreal fx1, fy1, fx2, fy2;
            if (noRotate) {
                fx1 = (bbox.left() * m(0, 0) + m(0, 3)) * invW;
                fy1 = (bbox.bottom() * m(1, 1) + m(1, 3)) * invW;
                fx2 = (bbox.right() * m(0, 0) + m(0, 3)) * invW;
                fy2 = (bbox.top() * m(1, 1) + m(1, 3)) * invW;
            } else {
                Q_ASSERT(isRotate90);
                fx1 = (bbox.bottom() * m(0, 1) + m(0, 3)) * invW;
                fy1 = (bbox.left() * m(1, 0) + m(1, 3)) * invW;
                fx2 = (bbox.top() * m(0, 1) + m(0, 3)) * invW;
                fy2 = (bbox.right() * m(1, 0) + m(1, 3)) * invW;
            }

            if (fx1 > fx2)
                qSwap(fx1, fx2);
            if (fy1 > fy2)
                qSwap(fy1, fy2);

            GLint ix1 = qRound((fx1 + 1) * m_device_rect.width() * qreal(0.5));
            GLint iy1 = qRound((fy1 + 1) * m_device_rect.height() * qreal(0.5));
            GLint ix2 = qRound((fx2 + 1) * m_device_rect.width() * qreal(0.5));
            GLint iy2 = qRound((fy2 + 1) * m_device_rect.height() * qreal(0.5));

            if (!(clipType & ScissorClip)) {
                m_current_scissor_rect = QRect(ix1, iy1, ix2 - ix1, iy2 - iy1);
                glEnable(GL_SCISSOR_TEST);
                clipType |= ScissorClip;
            } else {
                m_current_scissor_rect &= QRect(ix1, iy1, ix2 - ix1, iy2 - iy1);
            }
            glScissor(m_current_scissor_rect.x(), m_current_scissor_rect.y(),
                      m_current_scissor_rect.width(), m_current_scissor_rect.height());
        } else {
            if (!(clipType & StencilClip)) {
                if (!m_clip_program.isLinked()) {
                    QSGShaderSourceBuilder::initializeProgramFromFiles(
                        &m_clip_program,
                        QStringLiteral(":/scenegraph/shaders/stencilclip.vert"),
                        QStringLiteral(":/scenegraph/shaders/stencilclip.frag"));
                    m_clip_program.bindAttributeLocation("vCoord", 0);
                    m_clip_program.link();
                    m_clip_matrix_id = m_clip_program.uniformLocation("matrix");
                }

                glClearStencil(0);
                glClear(GL_STENCIL_BUFFER_BIT);
                glEnable(GL_STENCIL_TEST);
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                glDepthMask(GL_FALSE);

                if (m_vertex_buffer_bound) {
                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                    m_vertex_buffer_bound = false;
                }
                if (m_index_buffer_bound) {
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
                    m_index_buffer_bound = false;
                }

                m_clip_program.bind();
                m_clip_program.enableAttributeArray(0);

                clipType |= StencilClip;
            }

            glStencilFunc(GL_EQUAL, m_current_stencil_value, 0xff); // stencil test, ref, test mask
            glStencilOp(GL_KEEP, GL_KEEP, GL_INCR); // stencil fail, z fail, z pass

            const QSGGeometry *g = clip->geometry();
            Q_ASSERT(g->attributeCount() > 0);
            const QSGGeometry::Attribute *a = g->attributes();
            glVertexAttribPointer(0, a->tupleSize, a->type, GL_FALSE, g->sizeOfVertex(), g->vertexData());

            m_clip_program.setUniformValue(m_clip_matrix_id, m);
            if (g->indexCount()) {
                glDrawElements(g->drawingMode(), g->indexCount(), g->indexType(), g->indexData());
            } else {
                glDrawArrays(g->drawingMode(), 0, g->vertexCount());
            }

            ++m_current_stencil_value;
        }

        clip = clip->clipList();
    }

    if (clipType & StencilClip) {
        m_clip_program.disableAttributeArray(0);
        glStencilFunc(GL_EQUAL, m_current_stencil_value, 0xff); // stencil test, ref, test mask
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); // stencil fail, z fail, z pass
        bindable()->reactivate();
    } else {
        glDisable(GL_STENCIL_TEST);
    }

    return clipType;
}



static inline int size_of_type(GLenum type)
{
    static int sizes[] = {
        sizeof(char),
        sizeof(unsigned char),
        sizeof(short),
        sizeof(unsigned short),
        sizeof(int),
        sizeof(unsigned int),
        sizeof(float),
        2,
        3,
        4,
        sizeof(double)
    };
    Q_ASSERT(type >= GL_BYTE && type <= 0x140A); // the value of GL_DOUBLE
    return sizes[type - GL_BYTE];
}


class QSGRendererVBOGeometryData : public QSGGeometryData
{
public:
    QSGRendererVBOGeometryData()
        : vertexBuffer(0)
        , indexBuffer(0)
    {
    }

    ~QSGRendererVBOGeometryData()
    {
        QOpenGLContext *ctx = QOpenGLContext::currentContext();
        if (!ctx)
            return;
        QOpenGLFunctions *func = ctx->functions();
        if (vertexBuffer)
            func->glDeleteBuffers(1, &vertexBuffer);
        if (indexBuffer)
            func->glDeleteBuffers(1, &indexBuffer);
    }

    GLuint vertexBuffer;
    GLuint indexBuffer;

    static QSGRendererVBOGeometryData *get(const QSGGeometry *g) {
        QSGRendererVBOGeometryData *gd = static_cast<QSGRendererVBOGeometryData *>(QSGGeometryData::data(g));
        if (!gd) {
            gd = new QSGRendererVBOGeometryData;
            QSGGeometryData::install(g, gd);
        }
        return gd;
    }

};

static inline GLenum qt_drawTypeForPattern(QSGGeometry::DataPattern p)
{
    Q_ASSERT(p > 0 && p <= 3);
    static GLenum drawTypes[] = { 0,
                                  GL_STREAM_DRAW,
                                  GL_DYNAMIC_DRAW,
                                  GL_STATIC_DRAW
                            };
    return drawTypes[p];
}


/*!
    Issues the GL draw call for the geometry \a g using the material \a shader.

    The function assumes that attributes have been bound and set up prior
    to making this call.

    \internal
 */

void QSGRenderer::draw(const QSGMaterialShader *shader, const QSGGeometry *g)
{
    const void *vertexData;
    int vertexByteSize = g->vertexCount() * g->sizeOfVertex();
    if (g->vertexDataPattern() != QSGGeometry::AlwaysUploadPattern && vertexByteSize > 1024) {

        // The base pointer for a VBO is 0
        vertexData = 0;

        bool updateData = QSGGeometryData::hasDirtyVertexData(g);
        QSGRendererVBOGeometryData *gd = QSGRendererVBOGeometryData::get(g);
        if (!gd->vertexBuffer) {
            glGenBuffers(1, &gd->vertexBuffer);
            updateData = true;
        }

        glBindBuffer(GL_ARRAY_BUFFER, gd->vertexBuffer);
        m_vertex_buffer_bound = true;

        if (updateData) {
            glBufferData(GL_ARRAY_BUFFER, vertexByteSize, g->vertexData(),
                         qt_drawTypeForPattern(g->vertexDataPattern()));
            QSGGeometryData::clearDirtyVertexData(g);
        }

    } else {
        if (m_vertex_buffer_bound) {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            m_vertex_buffer_bound = false;
        }
        vertexData = g->vertexData();
    }

    // Bind the vertices to attributes...
    char const *const *attrNames = shader->attributeNames();
    int offset = 0;
    for (int j = 0; attrNames[j]; ++j) {
        if (!*attrNames[j])
            continue;
        Q_ASSERT_X(j < g->attributeCount(), "QSGRenderer::bindGeometry()", "Geometry lacks attribute required by material");
        const QSGGeometry::Attribute &a = g->attributes()[j];
        Q_ASSERT_X(j == a.position, "QSGRenderer::bindGeometry()", "Geometry does not have continuous attribute positions");

#if defined(QT_OPENGL_ES_2)
        GLboolean normalize = a.type != GL_FLOAT;
#else
        GLboolean normalize = a.type != GL_FLOAT && a.type != GL_DOUBLE;
#endif
        glVertexAttribPointer(a.position, a.tupleSize, a.type, normalize, g->sizeOfVertex(), (char *) vertexData + offset);
        offset += a.tupleSize * size_of_type(a.type);
    }

    // Set up the indices...
    const void *indexData;
    if (g->indexDataPattern() != QSGGeometry::AlwaysUploadPattern && g->indexCount() > 512) {

        // Base pointer for a VBO is 0
        indexData = 0;

        bool updateData = QSGGeometryData::hasDirtyIndexData(g);
        QSGRendererVBOGeometryData *gd = QSGRendererVBOGeometryData::get(g);
        if (!gd->indexBuffer) {
            glGenBuffers(1, &gd->indexBuffer);
            updateData = true;
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gd->indexBuffer);
        m_index_buffer_bound = true;

        if (updateData) {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         g->indexCount() * g->sizeOfIndex(),
                         g->indexData(),
                         qt_drawTypeForPattern(g->indexDataPattern()));
            QSGGeometryData::clearDirtyIndexData(g);
        }

    } else {
        if (m_index_buffer_bound) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            m_index_buffer_bound = false;
        }
        indexData = g->indexData();
    }

    // Set the line width if applicable
    if (g->drawingMode() == GL_LINES || g->drawingMode() == GL_LINE_STRIP || g->drawingMode() == GL_LINE_LOOP) {
        glLineWidth(g->lineWidth());
    }

    // draw the stuff...
    if (g->indexCount()) {
        glDrawElements(g->drawingMode(), g->indexCount(), g->indexType(), indexData);
    } else {
        glDrawArrays(g->drawingMode(), 0, g->vertexCount());
    }

    // We leave buffers bound for now... They will be reset by bind on next draw() or
    // set back to 0 if next draw is not using VBOs

}

/*!
    \class QSGNodeDumper
    \brief The QSGNodeDumper class provides a way of dumping a scene grahp to the console.

    This class is solely for debugging purposes.

    \internal
 */

void QSGNodeDumper::dump(QSGNode *n)
{
    QSGNodeDumper dump;
    dump.visitNode(n);
}

void QSGNodeDumper::visitNode(QSGNode *n)
{
    qDebug() << QByteArray(m_indent * 2, ' ').constData() << n;
    QSGNodeVisitor::visitNode(n);
}

void QSGNodeDumper::visitChildren(QSGNode *n)
{
    ++m_indent;
    QSGNodeVisitor::visitChildren(n);
    --m_indent;
}


QT_END_NAMESPACE
