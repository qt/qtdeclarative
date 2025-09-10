// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgrenderer_p.h"
#include "qsgnodeupdater_p.h"
#include <private/qquickprofiler_p.h>
#include <qtquick_tracepoints_p.h>

#include <QtCore/QElapsedTimer>

QT_BEGIN_NAMESPACE

static QElapsedTimer frameTimer;
static qint64 preprocessTime;
static qint64 updatePassTime;

Q_TRACE_POINT(qtquick, QSG_preprocess_entry)
Q_TRACE_POINT(qtquick, QSG_preprocess_exit)
Q_TRACE_POINT(qtquick, QSG_update_entry)
Q_TRACE_POINT(qtquick, QSG_update_exit)
Q_TRACE_POINT(qtquick, QSG_renderScene_entry)
Q_TRACE_POINT(qtquick, QSG_renderScene_exit)

int qt_sg_envInt(const char *name, int defaultValue)
{
    if (Q_LIKELY(!qEnvironmentVariableIsSet(name)))
        return defaultValue;
    bool ok = false;
    int value = qEnvironmentVariableIntValue(name, &ok);
    return ok ? value : defaultValue;
}

/*!
    \class QSGRenderer
    \brief The renderer class is the abstract baseclass used for rendering the
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
    : m_current_opacity(1)
    , m_current_determinant(1)
    , m_device_pixel_ratio(1)
    , m_context(context)
    , m_current_uniform_data(nullptr)
    , m_current_resource_update_batch(nullptr)
    , m_rhi(nullptr)
    , m_node_updater(nullptr)
    , m_changed_emitted(false)
    , m_is_rendering(false)
    , m_is_preprocessing(false)
{
    m_current_projection_matrix.resize(1);
    m_current_projection_matrix_native_ndc.resize(1);
}


QSGRenderer::~QSGRenderer()
{
    setRootNode(nullptr);
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

bool QSGRenderer::isMirrored() const
{
    QMatrix4x4 matrix = projectionMatrix(0);
    // Mirrored relative to the usual Qt coordinate system with origin in the top left corner.
    return matrix(0, 0) * matrix(1, 1) - matrix(0, 1) * matrix(1, 0) > 0;
}

void QSGRenderer::renderScene()
{
    if (!rootNode())
        return;

    Q_TRACE_SCOPE(QSG_renderScene);
    m_is_rendering = true;

    bool profileFrames = QSG_LOG_TIME_RENDERER().isDebugEnabled();
    if (profileFrames)
        frameTimer.start();
    Q_QUICK_SG_PROFILE_START(QQuickProfiler::SceneGraphRendererFrame);

    // The QML Profiler architecture is extremely fragile: we have to record a
    // hardcoded number of data points for each event, otherwise the view will
    // show weird things in Creator. So record a dummy Binding data point, even
    // though it is meaningless for our purposes.
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphRendererFrame,
                              QQuickProfiler::SceneGraphRendererBinding);

    qint64 renderTime = 0;

    preprocess();

    Q_TRACE(QSG_render_entry);
    render();
    if (profileFrames)
        renderTime = frameTimer.nsecsElapsed();
    Q_TRACE(QSG_render_exit);
    Q_QUICK_SG_PROFILE_END(QQuickProfiler::SceneGraphRendererFrame,
                           QQuickProfiler::SceneGraphRendererRender);

    m_is_rendering = false;
    m_changed_emitted = false;

    qCDebug(QSG_LOG_TIME_RENDERER,
            "time in renderer: total=%dms, preprocess=%d, updates=%d, rendering=%d",
            int(renderTime / 1000000),
            int(preprocessTime / 1000000),
            int((updatePassTime - preprocessTime) / 1000000),
            int((renderTime - updatePassTime) / 1000000));
}

void QSGRenderer::prepareSceneInline()
{
    if (!rootNode())
        return;

    Q_ASSERT(!m_is_rendering);
    m_is_rendering = true;

    preprocess();

    prepareInline();
}

void QSGRenderer::renderSceneInline()
{
    Q_ASSERT(m_is_rendering);

    renderInline();

    m_is_rendering = false;
    m_changed_emitted = false;
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

void QSGRenderer::preprocess()
{
    Q_TRACE(QSG_preprocess_entry);

    m_is_preprocessing = true;

    QSGRootNode *root = rootNode();
    Q_ASSERT(root);

    // We need to take a copy here, in case any of the preprocess calls deletes a node that
    // is in the preprocess list and thus, changes the m_nodes_to_preprocess behind our backs
    // For the default case, when this does not happen, the cost is negligible.
    QSet<QSGNode *> items = m_nodes_to_preprocess;

    m_context->preprocess();

    for (QSet<QSGNode *>::const_iterator it = items.constBegin();
         it != items.constEnd(); ++it) {
        QSGNode *n = *it;

        // If we are currently preprocessing, check this node hasn't been
        // deleted or something. we don't want a use-after-free!
        if (m_nodes_dont_preprocess.contains(n)) // skip
            continue;
        if (!nodeUpdater()->isNodeBlocked(n, root)) {
            n->preprocess();
        }
    }

    bool profileFrames = QSG_LOG_TIME_RENDERER().isDebugEnabled();
    if (profileFrames)
        preprocessTime = frameTimer.nsecsElapsed();
    Q_TRACE(QSG_preprocess_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphRendererFrame,
                              QQuickProfiler::SceneGraphRendererPreprocess);
    Q_TRACE(QSG_update_entry);

    nodeUpdater()->updateStates(root);

    if (profileFrames)
        updatePassTime = frameTimer.nsecsElapsed();
    Q_TRACE(QSG_update_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphRendererFrame,
                              QQuickProfiler::SceneGraphRendererUpdate);

    m_is_preprocessing = false;
    m_nodes_dont_preprocess.clear();
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
    if (node->flags() & QSGNode::UsePreprocess) {
        m_nodes_to_preprocess.remove(node);

        // If preprocessing *now*, mark the node as gone.
        if (m_is_preprocessing)
            m_nodes_dont_preprocess.insert(node);
    }
}

void QSGRenderer::prepareInline()
{
}

void QSGRenderer::renderInline()
{
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
