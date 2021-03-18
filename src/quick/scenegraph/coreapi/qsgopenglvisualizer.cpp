/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
** Copyright (C) 2016 Robin Burchell <robin.burchell@viroteck.net>
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

#include "qsgopenglvisualizer_p.h"
#include <qmath.h>
#include <private/qsgshadersourcebuilder_p.h>

QT_BEGIN_NAMESPACE

namespace QSGBatchRenderer
{

#define QSGNODE_TRAVERSE(NODE) for (QSGNode *child = NODE->firstChild(); child; child = child->nextSibling())
#define SHADOWNODE_TRAVERSE(NODE) for (Node *child = NODE->firstChild(); child; child = child->sibling())
#define QSGNODE_DIRTY_PARENT (QSGNode::DirtyNodeAdded \
                              | QSGNode::DirtyOpacity \
                              | QSGNode::DirtyMatrix \
                              | QSGNode::DirtyNodeRemoved)

QMatrix4x4 qsg_matrixForRoot(Node *node);

class VisualizeShader : public QOpenGLShaderProgram
{
public:
    int color;
    int matrix;
    int rotation;
    int pattern;
    int projection;
};

OpenGLVisualizer::OpenGLVisualizer(Renderer *renderer)
    : Visualizer(renderer),
      m_funcs(QOpenGLContext::currentContext()->functions()),
      m_visualizeProgram(nullptr)
{
}

OpenGLVisualizer::~OpenGLVisualizer()
{
    releaseResources();
}

void OpenGLVisualizer::releaseResources()
{
    delete m_visualizeProgram;
    m_visualizeProgram = nullptr;
}

void OpenGLVisualizer::prepareVisualize()
{
    // nothing to do here
}

void OpenGLVisualizer::visualizeDrawGeometry(const QSGGeometry *g)
{
    if (g->attributeCount() < 1)
        return;
    const QSGGeometry::Attribute *a = g->attributes();
    m_funcs->glVertexAttribPointer(0, a->tupleSize, a->type, false, g->sizeOfVertex(), g->vertexData());
    if (g->indexCount())
        m_funcs->glDrawElements(g->drawingMode(), g->indexCount(), g->indexType(), g->indexData());
    else
        m_funcs->glDrawArrays(g->drawingMode(), 0, g->vertexCount());

}

void OpenGLVisualizer::visualizeBatch(Batch *b)
{
    VisualizeShader *shader = static_cast<VisualizeShader *>(m_visualizeProgram);

    if (b->positionAttribute != 0)
        return;

    QSGGeometryNode *gn = b->first->node;
    QSGGeometry *g = gn->geometry();
    const QSGGeometry::Attribute &a = g->attributes()[b->positionAttribute];

    m_funcs->glBindBuffer(GL_ARRAY_BUFFER, b->vbo.id);

    QMatrix4x4 matrix(m_renderer->m_current_projection_matrix);
    if (b->root)
        matrix = matrix * qsg_matrixForRoot(b->root);

    shader->setUniformValue(shader->pattern, float(b->merged ? 0 : 1));

    QColor color = QColor::fromHsvF((rand() & 1023) / 1023.0, 1.0, 1.0);
    float cr = color.redF();
    float cg = color.greenF();
    float cb = color.blueF();
    shader->setUniformValue(shader->color, cr, cg, cb, 1.0);

    if (b->merged) {
        shader->setUniformValue(shader->matrix, matrix);
        const char *dataStart = m_renderer->m_context->separateIndexBuffer() ? b->ibo.data : b->vbo.data;
        for (int ds=0; ds<b->drawSets.size(); ++ds) {
            const DrawSet &set = b->drawSets.at(ds);
            m_funcs->glVertexAttribPointer(a.position, 2, a.type, false, g->sizeOfVertex(),
                                           (void *) (qintptr) (set.vertices));
            m_funcs->glDrawElements(g->drawingMode(), set.indexCount, GL_UNSIGNED_SHORT,
                                    (void *)(qintptr)(dataStart + set.indices));
        }
    } else {
        Element *e = b->first;
        int offset = 0;
        while (e) {
            gn = e->node;
            g = gn->geometry();
            shader->setUniformValue(shader->matrix, matrix * *gn->matrix());
            m_funcs->glVertexAttribPointer(a.position, a.tupleSize, a.type, false, g->sizeOfVertex(),
                                           (void *) (qintptr) offset);
            if (g->indexCount())
                m_funcs->glDrawElements(g->drawingMode(), g->indexCount(), g->indexType(), g->indexData());
            else
                m_funcs->glDrawArrays(g->drawingMode(), 0, g->vertexCount());
            offset += g->sizeOfVertex() * g->vertexCount();
            e = e->nextInBatch;
        }
    }
}

void OpenGLVisualizer::visualizeClipping(QSGNode *node)
{
    if (node->type() == QSGNode::ClipNodeType) {
        VisualizeShader *shader = static_cast<VisualizeShader *>(m_visualizeProgram);
        QSGClipNode *clipNode = static_cast<QSGClipNode *>(node);
        QMatrix4x4 matrix = m_renderer->m_current_projection_matrix;
        if (clipNode->matrix())
            matrix = matrix * *clipNode->matrix();
        shader->setUniformValue(shader->matrix, matrix);
        visualizeDrawGeometry(clipNode->geometry());
    }

    QSGNODE_TRAVERSE(node) {
        visualizeClipping(child);
    }
}

void OpenGLVisualizer::visualizeChanges(Node *n)
{

    if (n->type() == QSGNode::GeometryNodeType && n->element()->batch && m_visualizeChangeSet.contains(n)) {
        uint dirty = m_visualizeChangeSet.value(n);
        bool tinted = (dirty & QSGNODE_DIRTY_PARENT) != 0;

        VisualizeShader *shader = static_cast<VisualizeShader *>(m_visualizeProgram);
        QColor color = QColor::fromHsvF((rand() & 1023) / 1023.0, 0.3, 1.0);
        float ca = 0.5;
        float cr = color.redF() * ca;
        float cg = color.greenF() * ca;
        float cb = color.blueF() * ca;
        shader->setUniformValue(shader->color, cr, cg, cb, ca);
        shader->setUniformValue(shader->pattern, float(tinted ? 0.5 : 0));

        QSGGeometryNode *gn = static_cast<QSGGeometryNode *>(n->sgNode);

        QMatrix4x4 matrix = m_renderer->m_current_projection_matrix;
        if (n->element()->batch->root)
            matrix = matrix * qsg_matrixForRoot(n->element()->batch->root);
        matrix = matrix * *gn->matrix();
        shader->setUniformValue(shader->matrix, matrix);
        visualizeDrawGeometry(gn->geometry());

        // This is because many changes don't propegate their dirty state to the
        // parent so the node updater will not unset these states. They are
        // not used for anything so, unsetting it should have no side effects.
        n->dirtyState = {};
    }

    SHADOWNODE_TRAVERSE(n) {
        visualizeChanges(child);
    }
}

void OpenGLVisualizer::visualizeOverdraw_helper(Node *node)
{
    if (node->type() == QSGNode::GeometryNodeType && node->element()->batch) {
        VisualizeShader *shader = static_cast<VisualizeShader *>(m_visualizeProgram);
        QSGGeometryNode *gn = static_cast<QSGGeometryNode *>(node->sgNode);

        QMatrix4x4 matrix = m_renderer->m_current_projection_matrix;
        matrix(2, 2) = m_renderer->m_zRange;
        matrix(2, 3) = 1.0f - node->element()->order * m_renderer->m_zRange;

        if (node->element()->batch->root)
            matrix = matrix * qsg_matrixForRoot(node->element()->batch->root);
        matrix = matrix * *gn->matrix();
        shader->setUniformValue(shader->matrix, matrix);

        QColor color = node->element()->batch->isOpaque ? QColor::fromRgbF(0.3, 1.0, 0.3) : QColor::fromRgbF(1.0, 0.3, 0.3);
        float ca = 0.33f;
        shader->setUniformValue(shader->color, color.redF() * ca, color.greenF() * ca, color.blueF() * ca, ca);

        visualizeDrawGeometry(gn->geometry());
    }

    SHADOWNODE_TRAVERSE(node) {
        visualizeOverdraw_helper(child);
    }
}

void OpenGLVisualizer::visualizeOverdraw()
{
    VisualizeShader *shader = static_cast<VisualizeShader *>(m_visualizeProgram);
    shader->setUniformValue(shader->color, 0.5f, 0.5f, 1.0f, 1.0f);
    shader->setUniformValue(shader->projection, 1);

    m_funcs->glBlendFunc(GL_ONE, GL_ONE);

    static float step = 0;
    step += static_cast<float>(M_PI * 2 / 1000.);
    if (step > M_PI * 2)
        step = 0;
    float angle = 80.0 * std::sin(step);

    QMatrix4x4 xrot; xrot.rotate(20, 1, 0, 0);
    QMatrix4x4 zrot; zrot.rotate(angle, 0, 0, 1);
    QMatrix4x4 tx; tx.translate(0, 0, 1);

    QMatrix4x4 m;

//    m.rotate(180, 0, 1, 0);

    m.translate(0, 0.5, 4);
    m.scale(2, 2, 1);

    m.rotate(-30, 1, 0, 0);
    m.rotate(angle, 0, 1, 0);
    m.translate(0, 0, -1);

    shader->setUniformValue(shader->rotation, m);

    float box[] = {
        // lower
        -1, 1, 0,   1, 1, 0,
        -1, 1, 0,   -1, -1, 0,
        1, 1, 0,    1, -1, 0,
        -1, -1, 0,  1, -1, 0,

        // upper
        -1, 1, 1,   1, 1, 1,
        -1, 1, 1,   -1, -1, 1,
        1, 1, 1,    1, -1, 1,
        -1, -1, 1,  1, -1, 1,

        // sides
        -1, -1, 0,  -1, -1, 1,
        1, -1, 0,   1, -1, 1,
        -1, 1, 0,   -1, 1, 1,
        1, 1, 0,    1, 1, 1
    };
    m_funcs->glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, box);
    m_funcs->glLineWidth(2);
    m_funcs->glDrawArrays(GL_LINES, 0, 24);

    visualizeOverdraw_helper(m_renderer->m_nodes.value(m_renderer->rootNode()));
}

void OpenGLVisualizer::visualize()
{
    if (m_visualizeMode == VisualizeNothing)
        return;

    if (!m_visualizeProgram) {
        VisualizeShader *prog = new VisualizeShader();
        QSGShaderSourceBuilder::initializeProgramFromFiles(
            prog,
            QStringLiteral(":/qt-project.org/scenegraph/shaders/visualization.vert"),
            QStringLiteral(":/qt-project.org/scenegraph/shaders/visualization.frag"));
        prog->bindAttributeLocation("v", 0);
        prog->link();
        prog->bind();
        prog->color = prog->uniformLocation("color");
        prog->pattern = prog->uniformLocation("pattern");
        prog->projection = prog->uniformLocation("projection");
        prog->matrix = prog->uniformLocation("matrix");
        prog->rotation = prog->uniformLocation("rotation");
        m_visualizeProgram = prog;
    } else {
        m_visualizeProgram->bind();
    }
    VisualizeShader *shader = static_cast<VisualizeShader *>(m_visualizeProgram);

    m_funcs->glDisable(GL_DEPTH_TEST);
    m_funcs->glEnable(GL_BLEND);
    m_funcs->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    m_funcs->glEnableVertexAttribArray(0);

    // Blacken out the actual rendered content...
    float bgOpacity = 0.8f;
    if (m_visualizeMode == VisualizeBatches)
        bgOpacity = 1.0;
    float v[] = { -1, 1,   1, 1,   -1, -1,   1, -1 };
    shader->setUniformValue(shader->color, 0.0f, 0.0f, 0.0f, bgOpacity);
    shader->setUniformValue(shader->matrix, QMatrix4x4());
    shader->setUniformValue(shader->rotation, QMatrix4x4());
    shader->setUniformValue(shader->pattern, 0.0f);
    shader->setUniformValue(shader->projection, false);
    m_funcs->glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, v);
    m_funcs->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (m_visualizeMode == VisualizeBatches) {
        srand(0); // To force random colors to be roughly the same every time..
        for (int i = 0; i < m_renderer->m_opaqueBatches.size(); ++i)
            visualizeBatch(m_renderer->m_opaqueBatches.at(i));
        for (int i = 0; i < m_renderer->m_alphaBatches.size(); ++i)
            visualizeBatch(m_renderer->m_alphaBatches.at(i));
    } else if (m_visualizeMode == VisualizeClipping) {
        shader->setUniformValue(shader->pattern, 0.5f);
        shader->setUniformValue(shader->color, 0.2f, 0.0f, 0.0f, 0.2f);
        visualizeClipping(m_renderer->rootNode());
    } else if (m_visualizeMode == VisualizeChanges) {
        visualizeChanges(m_renderer->m_nodes.value(m_renderer->rootNode()));
        m_visualizeChangeSet.clear();
    } else if (m_visualizeMode == VisualizeOverdraw) {
        visualizeOverdraw();
    }

    // Reset state back to defaults..
    m_funcs->glDisable(GL_BLEND);
    m_funcs->glDisableVertexAttribArray(0);
    shader->release();
}

}

QT_END_NAMESPACE
