/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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


#define GL_GLEXT_PROTOTYPES

#include "qsgdefaultrenderer_p.h"
#include "qsgmaterial.h"

#include <QtCore/qvarlengtharray.h>
#include <QtGui/qguiapplication.h>
#include <QtCore/qpair.h>
#include <QtCore/QElapsedTimer>

//#define FORCE_NO_REORDER

// #define RENDERER_DEBUG
#ifdef RENDERER_DEBUG
#define DEBUG_THRESHOLD 0
QElapsedTimer debugTimer;
int materialChanges;
int geometryNodesDrawn;
#endif

QT_BEGIN_NAMESPACE

static bool nodeLessThan(QSGNode *nodeA, QSGNode *nodeB)
{
    if (nodeA->type() != nodeB->type())
        return nodeA->type() < nodeB->type();
    if (nodeA->type() != QSGNode::GeometryNodeType)
        return nodeA < nodeB;
    QSGGeometryNode *a = static_cast<QSGGeometryNode *>(nodeA);
    QSGGeometryNode *b = static_cast<QSGGeometryNode *>(nodeB);

    // Sort by clip...
    if (a->clipList() != b->clipList())
        return a->clipList() < b->clipList();

    // Sort by material definition
    QSGMaterialType *aDef = a->material()->type();
    QSGMaterialType *bDef = b->material()->type();

    if (aDef != bDef)
        return aDef < bDef;

    // Sort by material state
    int cmp = a->material()->compare(b->material());
    if (cmp != 0)
        return cmp < 0;

    return a->matrix() < b->matrix();
}

static bool nodeLessThanWithRenderOrder(QSGNode *nodeA, QSGNode *nodeB)
{
    if (nodeA->type() != nodeB->type())
        return nodeA->type() < nodeB->type();
    if (nodeA->type() != QSGNode::GeometryNodeType)
        return nodeA < nodeB;
    QSGGeometryNode *a = static_cast<QSGGeometryNode *>(nodeA);
    QSGGeometryNode *b = static_cast<QSGGeometryNode *>(nodeB);

    // Sort by clip...
    if (a->clipList() != b->clipList())
        return a->clipList() < b->clipList();

    // Sort by material definition
    QSGMaterialType *aDef = a->material()->type();
    QSGMaterialType *bDef = b->material()->type();

    if (!(a->material()->flags() & QSGMaterial::Blending)) {
        int aOrder = a->renderOrder();
        int bOrder = b->renderOrder();
        if (aOrder != bOrder)
            return aOrder > bOrder;
    }

    if (aDef != bDef)
        return aDef < bDef;

    // Sort by material state
    int cmp = a->material()->compare(b->material());
    if (cmp != 0)
        return cmp < 0;

    return a->matrix() < b->matrix();
}

QSGDefaultRenderer::QSGDefaultRenderer(QSGContext *context)
    : QSGRenderer(context)
    , m_opaqueNodes(64)
    , m_transparentNodes(64)
    , m_renderGroups(4)
    , m_rebuild_lists(false)
    , m_sort_front_to_back(false)
    , m_render_node_added(false)
    , m_currentRenderOrder(1)
{
#if defined(QML_RUNTIME_TESTING)
    QStringList args = qApp->arguments();
    m_render_opaque_nodes = !args.contains(QLatin1String("--no-opaque-nodes"));
    m_render_alpha_nodes = !args.contains(QLatin1String("--no-alpha-nodes"));
#endif
}

void QSGDefaultRenderer::nodeChanged(QSGNode *node, QSGNode::DirtyState state)
{
    QSGRenderer::nodeChanged(node, state);

    const quint32 rebuildBits = QSGNode::DirtyNodeAdded | QSGNode::DirtyNodeRemoved
                                | QSGNode::DirtyMaterial | QSGNode::DirtyOpacity
                                | QSGNode::DirtyForceUpdate;

    if (state & rebuildBits)
        m_rebuild_lists = true;
}

void QSGDefaultRenderer::render()
{
#if defined (QML_RUNTIME_TESTING)
    static bool dumpTree = qApp->arguments().contains(QLatin1String("--dump-tree"));
    if (dumpTree) {
        printf("\n\n");
        QSGNodeDumper::dump(rootNode());
    }
#endif

#ifdef RENDERER_DEBUG
    debugTimer.invalidate();
    debugTimer.start();
    geometryNodesDrawn = 0;
    materialChanges = 0;
#endif

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);

    glFrontFace(isMirrored() ? GL_CW : GL_CCW);
    glDisable(GL_CULL_FACE);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(true);
    glDepthFunc(GL_LESS);
#if defined(QT_OPENGL_ES)
    glClearDepthf(1);
#else
    glClearDepth(1);
#endif

    glDisable(GL_SCISSOR_TEST);
    glClearColor(m_clear_color.redF(), m_clear_color.greenF(), m_clear_color.blueF(), m_clear_color.alphaF());

#ifdef RENDERER_DEBUG
    int debugtimeSetup = debugTimer.elapsed();
#endif

    bindable()->clear(clearMode());

#ifdef RENDERER_DEBUG
    int debugtimeClear = debugTimer.elapsed();
#endif

    QRect r = viewportRect();
    glViewport(r.x(), deviceRect().bottom() - r.bottom(), r.width(), r.height());
    m_current_projection_matrix = projectionMatrix();
    m_current_model_view_matrix.setToIdentity();

    m_currentClip = 0;
    glDisable(GL_STENCIL_TEST);

    m_currentMaterial = 0;
    m_currentProgram = 0;
    m_currentMatrix = 0;

    bool sortNodes = m_rebuild_lists;

    if (m_rebuild_lists) {
        m_opaqueNodes.reset();
        m_transparentNodes.reset();
        m_renderGroups.reset();
        m_currentRenderOrder = 1;
        buildLists(rootNode());
        m_rebuild_lists = false;
        m_render_node_added = false;
        RenderGroup group = { m_opaqueNodes.size(), m_transparentNodes.size() };
        m_renderGroups.add(group);
    }

#ifdef RENDERER_DEBUG
    int debugtimeLists = debugTimer.elapsed();
#endif

    if (sortNodes) {
        if (!m_opaqueNodes.isEmpty()) {
            bool (*lessThan)(QSGNode *, QSGNode *);
            lessThan = m_sort_front_to_back ? nodeLessThanWithRenderOrder : nodeLessThan;
            int start = 0;
            for (int i = 0; i < m_renderGroups.size(); ++i) {
                int end = m_renderGroups.at(i).opaqueEnd;
                if (end != start)
                    qSort(&m_opaqueNodes.first() + start, &m_opaqueNodes.first() + end, lessThan);
                start = end;
            }
        }
    }

#ifdef RENDERER_DEBUG
    int debugtimeSorting = debugTimer.elapsed();
#endif

    int opaqueStart = 0;
    int transparentStart = 0;
    for (int i = 0; i < m_renderGroups.size(); ++i) {
        int opaqueEnd = m_renderGroups.at(i).opaqueEnd;
        int transparentEnd = m_renderGroups.at(i).transparentEnd;

        glDisable(GL_BLEND);
        glDepthMask(true);
#ifdef QML_RUNTIME_TESTING
        if (m_render_opaque_nodes)
#endif
        {
#if defined (QML_RUNTIME_TESTING)
            if (dumpTree)
                qDebug() << "Opaque Nodes:";
#endif
            if (opaqueEnd != opaqueStart)
                renderNodes(&m_opaqueNodes.first() + opaqueStart, opaqueEnd - opaqueStart);
        }

        glEnable(GL_BLEND);
        glDepthMask(false);
#ifdef QML_RUNTIME_TESTING
        if (m_render_alpha_nodes)
#endif
        {
#if defined (QML_RUNTIME_TESTING)
            if (dumpTree)
                qDebug() << "Alpha Nodes:";
#endif
            if (transparentEnd != transparentStart)
                renderNodes(&m_transparentNodes.first() + transparentStart, transparentEnd - transparentStart);
        }

        opaqueStart = opaqueEnd;
        transparentStart = transparentEnd;
    }

#ifdef RENDERER_DEBUG
    int debugtimeRender = debugTimer.elapsed();
#endif

    if (m_currentProgram)
        m_currentProgram->deactivate();

#ifdef RENDERER_DEBUG
    if (debugTimer.elapsed() > DEBUG_THRESHOLD) {
        printf(" --- Renderer breakdown:\n"
               "     - setup=%d, clear=%d, building=%d, sorting=%d, render=%d\n"
               "     - material changes: total=%d\n"
               "     - geometry nodes: total=%d\n",
               debugtimeSetup,
               debugtimeClear - debugtimeSetup,
               debugtimeLists - debugtimeClear,
               debugtimeSorting - debugtimeLists,
               debugtimeRender - debugtimeSorting,
               materialChanges,
               geometryNodesDrawn);
    }
#endif

}

void QSGDefaultRenderer::setSortFrontToBackEnabled(bool sort)
{
    printf("setting sorting to... %d\n", sort);
    m_sort_front_to_back = sort;
}

bool QSGDefaultRenderer::isSortFrontToBackEnabled() const
{
    return m_sort_front_to_back;
}

void QSGDefaultRenderer::buildLists(QSGNode *node)
{
    if (node->isSubtreeBlocked())
        return;

    if (node->type() == QSGNode::GeometryNodeType) {
        QSGGeometryNode *geomNode = static_cast<QSGGeometryNode *>(node);
        qreal opacity = geomNode->inheritedOpacity();
        QSGMaterial *m = geomNode->activeMaterial();

#ifdef FORCE_NO_REORDER
        if (true) {
#else
        if ((m->flags() & QSGMaterial::Blending) || opacity < 1) {
#endif
            geomNode->setRenderOrder(m_currentRenderOrder - 1);
            m_transparentNodes.add(geomNode);
        } else {
            if (m_render_node_added) {
                // Start new group of nodes so that this opaque node is render on top of the
                // render node.
                RenderGroup group = { m_opaqueNodes.size(), m_transparentNodes.size() };
                m_renderGroups.add(group);
                m_render_node_added = false;
            }
            geomNode->setRenderOrder(m_currentRenderOrder);
            m_opaqueNodes.add(geomNode);
            m_currentRenderOrder += 2;
        }
    } else if (node->type() == QSGNode::RenderNodeType) {
        QSGRenderNode *renderNode = static_cast<QSGRenderNode *>(node);
        m_transparentNodes.add(renderNode);
        m_render_node_added = true;
    }

    if (!node->firstChild())
        return;

    for (QSGNode *c = node->firstChild(); c; c = c->nextSibling())
        buildLists(c);
}

void QSGDefaultRenderer::renderNodes(QSGNode *const *nodes, int count)
{
    const float scale = 1.0f / m_currentRenderOrder;
    int currentRenderOrder = 0x80000000;
    ClipType currentClipType = NoClip;
    QMatrix4x4 projection = projectionMatrix();
    m_current_projection_matrix.setColumn(2, scale * projection.column(2));

    //int clipChangeCount = 0;
    //int programChangeCount = 0;
    //int materialChangeCount = 0;

    for (int i = 0; i < count; ++i) {
        if (nodes[i]->type() == QSGNode::RenderNodeType) {
            QSGRenderNode *renderNode = static_cast<QSGRenderNode *>(nodes[i]);

            if (m_currentProgram)
                m_currentProgram->deactivate();
            m_currentMaterial = 0;
            m_currentProgram = 0;
            m_currentMatrix = 0;
            currentRenderOrder = 0x80000000;

            bool changeClip = renderNode->clipList() != m_currentClip;
            // The clip function relies on there not being any depth testing..
            glDisable(GL_DEPTH_TEST);
            if (changeClip) {
                currentClipType = updateStencilClip(renderNode->clipList());
                m_currentClip = renderNode->clipList();
                //++clipChangeCount;
            }

            glDepthMask(false);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            QSGRenderNode::RenderState state;
            state.projectionMatrix = &projection;
            state.scissorEnabled = currentClipType & ScissorClip;
            state.stencilEnabled = currentClipType & StencilClip;
            state.scissorRect = m_current_scissor_rect;
            state.stencilValue = m_current_stencil_value;

            renderNode->render(state);

            QSGRenderNode::StateFlags changes = renderNode->changedStates();
            if (changes & QSGRenderNode::ViewportState) {
                QRect r = viewportRect();
                glViewport(r.x(), deviceRect().bottom() - r.bottom(), r.width(), r.height());
            }
            if (changes & QSGRenderNode::StencilState) {
                glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
                glStencilMask(0xff);
                glDisable(GL_STENCIL_TEST);
            }
            if (changes & (QSGRenderNode::StencilState | QSGRenderNode::ScissorState)) {
                glDisable(GL_SCISSOR_TEST);
                m_currentClip = 0;
                currentClipType = NoClip;
            }
            if (changes & QSGRenderNode::DepthState) {
#if defined(QT_OPENGL_ES)
                glClearDepthf(1);
#else
                glClearDepth(1);
#endif
                if (m_clear_mode & QSGRenderer::ClearDepthBuffer) {
                    glDepthMask(true);
                    glClear(GL_DEPTH_BUFFER_BIT);
                }
                glDepthMask(false);
                glDepthFunc(GL_LESS);
            }
            if (changes & QSGRenderNode::ColorState)
                bindable()->reactivate();
            if (changes & QSGRenderNode::BlendState) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            }
            if (changes & QSGRenderNode::CullState) {
                glFrontFace(isMirrored() ? GL_CW : GL_CCW);
                glDisable(GL_CULL_FACE);
            }

            glEnable(GL_DEPTH_TEST);

            m_current_model_view_matrix.setToIdentity();
            m_current_determinant = 1;
        } else if (nodes[i]->type() == QSGNode::GeometryNodeType) {
            QSGGeometryNode *geomNode = static_cast<QSGGeometryNode *>(nodes[i]);

            QSGMaterialShader::RenderState::DirtyStates updates;

#if defined (QML_RUNTIME_TESTING)
            static bool dumpTree = qApp->arguments().contains(QLatin1String("--dump-tree"));
            if (dumpTree)
                qDebug() << geomNode;
#endif

            bool changeMatrix = m_currentMatrix != geomNode->matrix();

            if (changeMatrix) {
                m_currentMatrix = geomNode->matrix();
                if (m_currentMatrix)
                    m_current_model_view_matrix = *m_currentMatrix;
                else
                    m_current_model_view_matrix.setToIdentity();
                m_current_determinant = m_current_model_view_matrix.determinant();
                updates |= QSGMaterialShader::RenderState::DirtyMatrix;
            }

            bool changeOpacity = m_current_opacity != geomNode->inheritedOpacity();
            if (changeOpacity) {
                updates |= QSGMaterialShader::RenderState::DirtyOpacity;
                m_current_opacity = geomNode->inheritedOpacity();
            }

            Q_ASSERT(geomNode->activeMaterial());

            QSGMaterial *material = geomNode->activeMaterial();
            QSGMaterialShader *program = m_context->prepareMaterial(material);
            Q_ASSERT(program->program()->isLinked());

            bool changeClip = geomNode->clipList() != m_currentClip;
            if (changeClip) {
                // The clip function relies on there not being any depth testing..
                glDisable(GL_DEPTH_TEST);
                currentClipType = updateStencilClip(geomNode->clipList());
                glEnable(GL_DEPTH_TEST);
                m_currentClip = geomNode->clipList();
#ifdef FORCE_NO_REORDER
                glDepthMask(false);
#else
                glDepthMask((material->flags() & QSGMaterial::Blending) == 0 && m_current_opacity == 1);
#endif
                //++clipChangeCount;
            }

            bool changeProgram = (changeClip && (currentClipType & StencilClip)) || m_currentProgram != program;
            if (changeProgram) {
                if (m_currentProgram)
                    m_currentProgram->deactivate();
                m_currentProgram = program;
                m_currentProgram->activate();
                //++programChangeCount;
                updates |= (QSGMaterialShader::RenderState::DirtyMatrix | QSGMaterialShader::RenderState::DirtyOpacity);

#ifdef RENDERER_DEBUG
                materialChanges++;
#endif
            }

            bool changeRenderOrder = currentRenderOrder != geomNode->renderOrder();
            if (changeRenderOrder) {
                currentRenderOrder = geomNode->renderOrder();
                m_current_projection_matrix.setColumn(3, projection.column(3)
                                                      + (m_currentRenderOrder - 1 - 2 * currentRenderOrder)
                                                      * m_current_projection_matrix.column(2));
                updates |= QSGMaterialShader::RenderState::DirtyMatrix;
            }

            if (changeProgram || m_currentMaterial != material) {
                program->updateState(state(updates), material, changeProgram ? 0 : m_currentMaterial);
                m_currentMaterial = material;
                //++materialChangeCount;
            }

            //glDepthRange((geomNode->renderOrder() + 0.1) * scale, (geomNode->renderOrder() + 0.9) * scale);

            const QSGGeometry *g = geomNode->geometry();
            draw(program, g);

#ifdef RENDERER_DEBUG
            geometryNodesDrawn++;
#endif
        }
    }
    //qDebug("Clip: %i, shader program: %i, material: %i times changed while drawing %s items",
    //    clipChangeCount, programChangeCount, materialChangeCount,
    //    &list == &m_transparentNodes ? "transparent" : "opaque");
}

QT_END_NAMESPACE
