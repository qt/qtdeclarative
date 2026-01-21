// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickshapecurverenderer_p.h"
#include "qquickshapecurverenderer_p_p.h"

#if QT_CONFIG(thread)
#include <QtCore/qthreadpool.h>
#endif

#include <QtGui/qvector2d.h>
#include <QtGui/qvector4d.h>
#include <QtGui/private/qtriangulator_p.h>
#include <QtGui/private/qtriangulatingstroker_p.h>
#include <QtGui/private/qrhi_p.h>

#include <QtQuick/private/qsgcurvefillnode_p.h>
#include <QtQuick/private/qsgcurvestrokenode_p.h>
#include <QtQuick/private/qquadpath_p.h>
#include <QtQuick/private/qsgcurveprocessor_p.h>
#include <QtQuick/qsgmaterial.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcShapeCurveRenderer, "qt.shape.curverenderer");

namespace {

/*! \internal
    Choice of vertex shader to use for the wireframe node:
    * \c SimpleWFT is for when vertices are already in logical coordinates
    * \c StrokeWFT chooses the stroke shader, which moves vertices according to the stroke width uniform
*/
enum WireFrameType { SimpleWFT, StrokeWFT };

class QQuickShapeWireFrameMaterialShader : public QSGMaterialShader
{
public:
    QQuickShapeWireFrameMaterialShader(WireFrameType wft, int viewCount) : m_wftype(wft)
    {
        setShaderFileName(VertexStage, wft == StrokeWFT ?
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/shapestroke_wireframe.vert.qsb") :
                          QStringLiteral(":/qt-project.org/shapes/shaders_ng/wireframe.vert.qsb"), viewCount);
        setShaderFileName(FragmentStage,
                          wft == StrokeWFT ?
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/shapestroke_wireframe.frag.qsb") :
                          QStringLiteral(":/qt-project.org/shapes/shaders_ng/wireframe.frag.qsb"), viewCount);
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *) override;

    WireFrameType m_wftype;
};

class QQuickShapeWireFrameMaterial : public QSGMaterial
{
public:
    QQuickShapeWireFrameMaterial(WireFrameType wft) : m_wftype(wft)
    {
        setFlag(Blending, true);
    }

    int compare(const QSGMaterial *other) const override
    {
        return (type() - other->type());
    }

    void setCosmeticStroke(bool c)
    {
        m_cosmeticStroke = c;
    }

    void setStrokeWidth(float width)
    {
        m_strokeWidth = width;
    }

    float strokeWidth()
    {
        return (m_cosmeticStroke ? -1.0 : 1.0) * qAbs(m_strokeWidth);
    }

protected:
    QSGMaterialType *type() const override
    {
        static QSGMaterialType t;
        return &t;
    }
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override
    {
        return new QQuickShapeWireFrameMaterialShader(m_wftype, viewCount());
    }

    WireFrameType m_wftype;
    bool m_cosmeticStroke = false;
    float m_strokeWidth = 1.0f;
};

bool QQuickShapeWireFrameMaterialShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *)
{
    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 64);
    const int matrixCount = qMin(state.projectionMatrixCount(), newMaterial->viewCount());
    bool changed = false;
    float localScale = /* newNode != nullptr ? newNode->localScale() : */ 1.0f;

    for (int viewIndex = 0; viewIndex < matrixCount; ++viewIndex) {
        if (state.isMatrixDirty()) {
            QMatrix4x4 m = state.combinedMatrix(viewIndex);
            if (m_wftype == StrokeWFT)
                m.scale(localScale);
            memcpy(buf->data() + 64 * viewIndex, m.constData(), 64);
            changed = true;
        }
    }
    // determinant is xscale * yscale, as long as Item.transform does not include shearing or rotation
    const float matrixScale = qSqrt(qAbs(state.determinant())) * state.devicePixelRatio() * localScale;
    memcpy(buf->data() + matrixCount * 64, &matrixScale, 4);
    const float dpr = state.devicePixelRatio();
    memcpy(buf->data() + matrixCount * 64 + 8, &dpr, 4);
    const float opacity = 1.0; // don't fade the wireframe
    memcpy(buf->data() + matrixCount * 64 + 4, &opacity, 4);
    const float strokeWidth = static_cast<QQuickShapeWireFrameMaterial *>(newMaterial)->strokeWidth();
    memcpy(buf->data() + matrixCount * 64 + 12, &strokeWidth, 4);
    changed = true;
    // shapestroke_wireframe.vert doesn't use the strokeColor and debug uniforms, so we don't bother setting them

    return changed;
}

template <WireFrameType wftype>
class QQuickShapeWireFrameNode : public QSGCurveAbstractNode
{
public:
    struct WireFrameVertex
    {
        float x, y, u, v, w, nx, ny, sw;
    };

    QQuickShapeWireFrameNode()
    {
        isDebugNode = true;
        setFlag(OwnsGeometry, true);
        setGeometry(new QSGGeometry(attributes(), 0, 0));
        activateMaterial();
    }

    void setColor(QColor col) override
    {
        Q_UNUSED(col);
    }

    void setUseStandardDerivatives(bool useStandardDerivatives) override
    {
        Q_UNUSED(useStandardDerivatives);
    }

    void setCosmeticStroke(bool c)
    {
        m_material->setCosmeticStroke(c);
    }

    void setStrokeWidth(float width)
    {
        m_material->setStrokeWidth(width);
    }

    void activateMaterial()
    {
        m_material.reset(new QQuickShapeWireFrameMaterial(wftype));
        setMaterial(m_material.data());
    }

    static const QSGGeometry::AttributeSet &attributes()
    {
        static QSGGeometry::Attribute data[] = {
            QSGGeometry::Attribute::createWithAttributeType(0, 2, QSGGeometry::FloatType, QSGGeometry::PositionAttribute),
            QSGGeometry::Attribute::createWithAttributeType(1, 3, QSGGeometry::FloatType, QSGGeometry::TexCoordAttribute),
            QSGGeometry::Attribute::createWithAttributeType(2, 3, QSGGeometry::FloatType, QSGGeometry::TexCoordAttribute),
        };
        static QSGGeometry::AttributeSet attrs = { 3, sizeof(WireFrameVertex), data };
        return attrs;
    }

    void cookGeometry() override
    {
        // Intentionally empty
    }

protected:
    QScopedPointer<QQuickShapeWireFrameMaterial> m_material;
};
}

QQuickShapeCurveRenderer::~QQuickShapeCurveRenderer()
{
    for (const PathData &pd : std::as_const(m_paths)) {
        if (pd.currentRunner) {
            pd.currentRunner->orphaned = true;
            if (!pd.currentRunner->isAsync || pd.currentRunner->isDone)
                delete pd.currentRunner;
        }
    }
}

void QQuickShapeCurveRenderer::beginSync(int totalCount, bool *countChanged)
{
    if (countChanged != nullptr && totalCount != m_paths.size())
        *countChanged = true;
    for (int i = totalCount; i < m_paths.size(); i++) { // Handle removal of paths
        setFillTextureProvider(i, nullptr); // deref window
        m_removedPaths.append(m_paths.at(i));
    }
    m_paths.resize(totalCount);
}

void QQuickShapeCurveRenderer::setPath(int index, const QPainterPath &path, QQuickShapePath::PathHints pathHints)
{
    auto &pathData = m_paths[index];
    pathData.originalPath = path;
    pathData.pathHints = pathHints;
    pathData.m_dirty |= PathDirty;
}

void QQuickShapeCurveRenderer::setStrokeColor(int index, const QColor &color)
{
    auto &pathData = m_paths[index];
    const bool wasVisible = pathData.isStrokeVisible();
    pathData.pen.setColor(color);
    if (pathData.isStrokeVisible() != wasVisible)
        pathData.m_dirty |= StrokeDirty;
    else
        pathData.m_dirty |= UniformsDirty;
}

void QQuickShapeCurveRenderer::setStrokeWidth(int index, qreal w)
{
    auto &pathData = m_paths[index];
    if (w > 0) {
        pathData.validPenWidth = true;
        pathData.pen.setWidthF(w);
    } else {
        pathData.validPenWidth = false;
    }
    pathData.m_dirty |= StrokeDirty;
}

void QQuickShapeCurveRenderer::setCosmeticStroke(int index, bool c)
{
    auto &pathData = m_paths[index];
    pathData.pen.setCosmetic(c);
    pathData.m_dirty |= StrokeDirty;
}

void QQuickShapeCurveRenderer::setFillColor(int index, const QColor &color)
{
    auto &pathData = m_paths[index];
    const bool wasVisible = pathData.isFillVisible();
    pathData.fillColor = color;
    if (pathData.isFillVisible() != wasVisible)
        pathData.m_dirty |= FillDirty;
    else
        pathData.m_dirty |= UniformsDirty;
}

void QQuickShapeCurveRenderer::setFillRule(int index, QQuickShapePath::FillRule fillRule)
{
    auto &pathData = m_paths[index];
    pathData.fillRule = Qt::FillRule(fillRule);
    pathData.m_dirty |= PathDirty;
}

void QQuickShapeCurveRenderer::setJoinStyle(int index,
                                          QQuickShapePath::JoinStyle joinStyle,
                                          int miterLimit)
{
    auto &pathData = m_paths[index];
    pathData.pen.setJoinStyle(Qt::PenJoinStyle(joinStyle));
    pathData.pen.setMiterLimit(miterLimit);
    pathData.m_dirty |= StrokeDirty;
}

void QQuickShapeCurveRenderer::setCapStyle(int index, QQuickShapePath::CapStyle capStyle)
{
    auto &pathData = m_paths[index];
    pathData.pen.setCapStyle(Qt::PenCapStyle(capStyle));
    pathData.m_dirty |= StrokeDirty;
}

void QQuickShapeCurveRenderer::setStrokeStyle(int index,
                                            QQuickShapePath::StrokeStyle strokeStyle,
                                            qreal dashOffset,
                                            const QList<qreal> &dashPattern)
{
    auto &pathData = m_paths[index];
    pathData.pen.setStyle(Qt::PenStyle(strokeStyle));
    if (strokeStyle == QQuickShapePath::DashLine) {
        pathData.pen.setDashPattern(dashPattern);
        pathData.pen.setDashOffset(dashOffset);
    }
    pathData.m_dirty |= StrokeDirty;
}

void QQuickShapeCurveRenderer::setFillGradient(int index, QQuickShapeGradient *gradient)
{
    PathData &pd(m_paths[index]);
    const bool wasVisible = pd.isFillVisible();
    pd.gradientType = QGradient::NoGradient;
    if (QQuickShapeLinearGradient *g  = qobject_cast<QQuickShapeLinearGradient *>(gradient)) {
        pd.gradientType = QGradient::LinearGradient;
        pd.gradient.stops = gradient->gradientStops();
        pd.gradient.spread = QGradient::Spread(gradient->spread());
        pd.gradient.a = QPointF(g->x1(), g->y1());
        pd.gradient.b = QPointF(g->x2(), g->y2());
    } else if (QQuickShapeRadialGradient *g = qobject_cast<QQuickShapeRadialGradient *>(gradient)) {
        pd.gradientType = QGradient::RadialGradient;
        pd.gradient.a = QPointF(g->centerX(), g->centerY());
        pd.gradient.b = QPointF(g->focalX(), g->focalY());
        pd.gradient.v0 = g->centerRadius();
        pd.gradient.v1 = g->focalRadius();
    } else if (QQuickShapeConicalGradient *g = qobject_cast<QQuickShapeConicalGradient *>(gradient)) {
        pd.gradientType = QGradient::ConicalGradient;
        pd.gradient.a = QPointF(g->centerX(), g->centerY());
        pd.gradient.v0 = g->angle();
    } else if (gradient != nullptr) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            qCWarning(lcShapeCurveRenderer) << "Unsupported gradient fill";
        }
    }

    if (pd.gradientType != QGradient::NoGradient) {
        pd.gradient.stops = gradient->gradientStops();
        pd.gradient.spread = QGradient::Spread(gradient->spread());
    }

    pd.m_dirty |= (pd.isFillVisible() != wasVisible) ? FillDirty : UniformsDirty;
}

void QQuickShapeCurveRenderer::setFillTransform(int index, const QSGTransform &transform)
{
    auto &pathData = m_paths[index];
    pathData.fillTransform = transform;
    pathData.m_dirty |= UniformsDirty;
}

void QQuickShapeCurveRenderer::setFillTextureProvider(int index, QQuickItem *textureProviderItem)
{
    auto &pathData = m_paths[index];
    const bool wasVisible = pathData.isFillVisible();
    if (pathData.fillTextureProviderItem != nullptr)
        QQuickItemPrivate::get(pathData.fillTextureProviderItem)->derefWindow();
    pathData.fillTextureProviderItem = textureProviderItem;
    if (pathData.fillTextureProviderItem != nullptr)
        QQuickItemPrivate::get(pathData.fillTextureProviderItem)->refWindow(m_item->window());
    pathData.m_dirty |= (pathData.isFillVisible() != wasVisible) ? FillDirty : UniformsDirty;
}

void QQuickShapeCurveRenderer::handleSceneChange(QQuickWindow *window)
{
    for (auto &pathData : m_paths) {
        if (pathData.fillTextureProviderItem != nullptr) {
            if (window == nullptr)
                QQuickItemPrivate::get(pathData.fillTextureProviderItem)->derefWindow();
            else
                QQuickItemPrivate::get(pathData.fillTextureProviderItem)->refWindow(window);
        }
    }
}

void QQuickShapeCurveRenderer::setAsyncCallback(void (*callback)(void *), void *data)
{
    m_asyncCallback = callback;
    m_asyncCallbackData = data;
}

void QQuickShapeCurveRenderer::endSync(bool async)
{
    bool asyncThreadsRunning = false;

    for (PathData &pathData : m_paths) {
        if (!pathData.m_dirty)
            continue;

        if (pathData.m_dirty == UniformsDirty) {
            // Requires no curve node computation, gets handled directly in updateNode()
            continue;
        }

        if (pathData.currentRunner) {
            // We are in a new sync round before updateNode() has been called to commit the results
            // of the previous sync and processing round
            if (pathData.currentRunner->isAsync) {
                // Already performing async processing. A new run of the runner will be started in
                // updateNode() to take care of the new dirty flags
                asyncThreadsRunning = true;
                continue;
            } else {
                // Throw away outdated results and start a new processing
                delete pathData.currentRunner;
                pathData.currentRunner = nullptr;
            }
        }

        pathData.currentRunner = new QQuickShapeCurveRunnable;
        setUpRunner(&pathData);

#if QT_CONFIG(thread)
        if (async) {
            pathData.currentRunner->isAsync = true;
            QThreadPool::globalInstance()->start(pathData.currentRunner);
            asyncThreadsRunning = true;
        } else
#endif
        {
            pathData.currentRunner->run();
        }
    }

    if (async && !asyncThreadsRunning && m_asyncCallback)
        m_asyncCallback(m_asyncCallbackData);
}

void QQuickShapeCurveRenderer::setUpRunner(PathData *pathData)
{
    Q_ASSERT(pathData->currentRunner);
    QQuickShapeCurveRunnable *runner = pathData->currentRunner;
    runner->isDone = false;
    runner->pathData = *pathData;
    runner->pathData.fillNodes.clear();
    runner->pathData.strokeNodes.clear();
    runner->pathData.currentRunner = nullptr;
    pathData->m_dirty = 0;
    if (!runner->isInitialized) {
        runner->isInitialized = true;
        runner->setAutoDelete(false);
        QObject::connect(runner, &QQuickShapeCurveRunnable::done, qApp,
                         [this](QQuickShapeCurveRunnable *r) {
                             r->isDone = true;
                             if (r->orphaned) {
                                 delete r; // Renderer was destroyed
                             } else if (r->isAsync) {
                                 maybeUpdateAsyncItem();
                             }
                         });
    }
}

void QQuickShapeCurveRenderer::maybeUpdateAsyncItem()
{
    for (const PathData &pd : std::as_const(m_paths)) {
        if (pd.currentRunner && !pd.currentRunner->isDone)
            return;
    }
    if (m_item)
        m_item->update();
    if (m_asyncCallback)
        m_asyncCallback(m_asyncCallbackData);
}

QQuickShapeCurveRunnable::~QQuickShapeCurveRunnable()
{
    qDeleteAll(pathData.fillNodes);
    qDeleteAll(pathData.strokeNodes);
}

void QQuickShapeCurveRunnable::run()
{
    QQuickShapeCurveRenderer::processPath(&pathData);
    emit done(this);
}

static bool disableScreenSpaceDerivativeShader()
{
    static bool d = qEnvironmentVariableIntValue("QT_QUICKSHAPES_DISABLE_STANDARD_DERIVATIVES") != 0;
    return d;
}

void QQuickShapeCurveRenderer::updateNode()
{
    if (!m_rootNode)
        return;

    auto updateUniforms = [](const PathData &pathData) {
        for (auto &pathNode : std::as_const(pathData.fillNodes)) {
            if (pathNode->isDebugNode)
                continue;
            QSGCurveFillNode *fillNode = static_cast<QSGCurveFillNode *>(pathNode);
            fillNode->setColor(pathData.fillColor);
            fillNode->setGradientType(pathData.gradientType);
            fillNode->setFillGradient(pathData.gradient);
            fillNode->setFillTransform(pathData.fillTransform);
            fillNode->setFillTextureProvider(pathData.fillTextureProviderItem != nullptr
                                             ? pathData.fillTextureProviderItem->textureProvider()
                                             : nullptr);
        }
        for (QSGCurveAbstractNode *pathNode : std::as_const(pathData.strokeNodes)) {
            pathNode->setColor(pathData.pen.color());
            if (pathNode->isDebugNode) {
                auto *wfNode = static_cast<QQuickShapeWireFrameNode<StrokeWFT> *>(pathNode);
                wfNode->setStrokeWidth(pathData.pen.widthF());
                wfNode->setCosmeticStroke(pathData.pen.isCosmetic());
            } else {
                auto *strokeNode = static_cast<QSGCurveStrokeNode *>(pathNode);
                strokeNode->setStrokeWidth(pathData.pen.widthF());
                strokeNode->setCosmeticStroke(pathData.pen.isCosmetic());
            }
        }
    };

    NodeList toBeDeleted;

    for (const PathData &pathData : std::as_const(m_removedPaths)) {
        toBeDeleted += pathData.fillNodes;
        toBeDeleted += pathData.strokeNodes;
    }
    m_removedPaths.clear();

    const bool supportsDerivatives = m_item != nullptr
                                     && m_item->window() != nullptr
                                     && m_item->window()->rhi() != nullptr
                                     && !disableScreenSpaceDerivativeShader()
            ? m_item->window()->rhi()->isFeatureSupported(QRhi::ScreenSpaceDerivatives)
            : false;

    for (int i = 0; i < m_paths.size(); i++) {
        PathData &pathData = m_paths[i];
        if (pathData.currentRunner) {
            if (!pathData.currentRunner->isDone)
                continue;
            // Find insertion point for new nodes. Default is the first stroke node of this path
            QSGNode *nextNode = pathData.strokeNodes.value(0);
            // If that is 0, use the first node (stroke or fill) of later paths, if any
            for (int j = i + 1; !nextNode && j < m_paths.size(); j++) {
                const PathData &pd = m_paths[j];
                nextNode = pd.fillNodes.isEmpty() ? pd.strokeNodes.value(0) : pd.fillNodes.value(0);
            }

            PathData &newData = pathData.currentRunner->pathData;
            if (newData.m_dirty & PathDirty)
                pathData.path = newData.path;
            if (newData.m_dirty & FillDirty) {
                pathData.fillPath = newData.fillPath;
                for (auto *node : std::as_const(newData.fillNodes)) {
                    node->setUseStandardDerivatives(supportsDerivatives);
                    if (nextNode)
                        m_rootNode->insertChildNodeBefore(node, nextNode);
                    else
                        m_rootNode->appendChildNode(node);
                }
                toBeDeleted += pathData.fillNodes;
                pathData.fillNodes = newData.fillNodes;
            }
            if (newData.m_dirty & StrokeDirty) {
                for (auto *node : std::as_const(newData.strokeNodes)) {
                    node->setUseStandardDerivatives(supportsDerivatives);
                    if (nextNode)
                        m_rootNode->insertChildNodeBefore(node, nextNode);
                    else
                        m_rootNode->appendChildNode(node);
                }
                toBeDeleted += pathData.strokeNodes;
                pathData.strokeNodes = newData.strokeNodes;
            }
            if (newData.m_dirty & UniformsDirty)
                updateUniforms(newData);

            // Ownership of new nodes have been transferred to root node
            newData.fillNodes.clear();
            newData.strokeNodes.clear();

#if QT_CONFIG(thread)
            if (pathData.currentRunner->isAsync && (pathData.m_dirty & ~UniformsDirty)) {
                // New changes have arrived while runner was computing; restart it to handle them
                setUpRunner(&pathData);
                QThreadPool::globalInstance()->start(pathData.currentRunner);
            } else
#endif
            {
                pathData.currentRunner->deleteLater();
                pathData.currentRunner = nullptr;
            }
        }

        if (pathData.m_dirty == UniformsDirty && !pathData.currentRunner) {
            // Simple case so no runner was created in endSync(); handle it directly here
            updateUniforms(pathData);
            pathData.m_dirty = 0;
        }
    }
    qDeleteAll(toBeDeleted); // also removes them from m_rootNode's child list
}

void QQuickShapeCurveRenderer::processPath(PathData *pathData)
{
    static const bool doOverlapSolving = !qEnvironmentVariableIntValue("QT_QUICKSHAPES_DISABLE_OVERLAP_SOLVER");
    static const bool doIntersetionSolving = !qEnvironmentVariableIntValue("QT_QUICKSHAPES_DISABLE_INTERSECTION_SOLVER");
    static const bool useTriangulatingStroker = qEnvironmentVariableIntValue("QT_QUICKSHAPES_TRIANGULATING_STROKER");
    static const bool simplifyPath = qEnvironmentVariableIntValue("QT_QUICKSHAPES_SIMPLIFY_PATHS");

    int &dirtyFlags = pathData->m_dirty;

    if (dirtyFlags & PathDirty) {
        if (simplifyPath)
            pathData->path = QQuadPath::fromPainterPath(pathData->originalPath.simplified(), QQuadPath::PathLinear | QQuadPath::PathNonIntersecting | QQuadPath::PathNonOverlappingControlPointTriangles);
        else
            pathData->path = QQuadPath::fromPainterPath(pathData->originalPath, QQuadPath::PathHints(int(pathData->pathHints)));
        pathData->path.setFillRule(pathData->fillRule);
        pathData->fillPath = {};
        dirtyFlags |= (FillDirty | StrokeDirty);
    }

    if (dirtyFlags & FillDirty) {
        if (pathData->isFillVisible()) {
            if (pathData->fillPath.isEmpty()) {
                pathData->fillPath = pathData->path.subPathsClosed();
                if (doIntersetionSolving)
                    QSGCurveProcessor::solveIntersections(pathData->fillPath);
                pathData->fillPath.addCurvatureData();
                if (doOverlapSolving)
                    QSGCurveProcessor::solveOverlaps(pathData->fillPath);
            }
            pathData->fillNodes = addFillNodes(pathData->fillPath);
            dirtyFlags |= (StrokeDirty | UniformsDirty);
        }
    }

    if (dirtyFlags & StrokeDirty) {
        if (pathData->isStrokeVisible()) {
            const QPen &pen = pathData->pen;
            const bool solid = (pen.style() == Qt::SolidLine);
            const QQuadPath &strokePath = solid ? pathData->path
                                                : pathData->path.dashed(pen.widthF(),
                                                                        pen.dashPattern(),
                                                                        pen.dashOffset());
            if (useTriangulatingStroker)
                pathData->strokeNodes = addTriangulatingStrokerNodes(strokePath, pen);
            else
                pathData->strokeNodes = addCurveStrokeNodes(strokePath, pen);
        }
    }
}

QQuickShapeCurveRenderer::NodeList QQuickShapeCurveRenderer::addFillNodes(const QQuadPath &path)
{
    NodeList ret;
    std::unique_ptr<QSGCurveFillNode> node(new QSGCurveFillNode);
    std::unique_ptr<QQuickShapeWireFrameNode<SimpleWFT>> wfNode;

    const qsizetype approxDataCount = 20 * path.elementCount();
    node->reserve(approxDataCount);

    const int debugFlags = debugVisualization();
    const bool wireFrame = debugFlags & DebugWireframe;

    if (Q_LIKELY(!wireFrame)) {
        QSGCurveProcessor::processFill(path,
                                       path.fillRule(),
                                       [&node](const std::array<QVector2D, 3> &v,
                                               const std::array<QVector2D, 3> &n,
                                               QSGCurveProcessor::uvForPointCallback uvForPoint)
                                       {
                                           node->appendTriangle(v, n, uvForPoint);
                                       });
    } else {
        QList<QQuickShapeWireFrameNode<SimpleWFT>::WireFrameVertex> wfVertices;
        wfVertices.reserve(approxDataCount);
        QSGCurveProcessor::processFill(path,
                                       path.fillRule(),
                                       [&wfVertices, &node](const std::array<QVector2D, 3> &v,
                                                            const std::array<QVector2D, 3> &n,
                                                            QSGCurveProcessor::uvForPointCallback uvForPoint)
                                       {
                                           node->appendTriangle(v, n, uvForPoint);

                                           wfVertices.append({v.at(0).x(), v.at(0).y(), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f }); // 0
                                           wfVertices.append({v.at(1).x(), v.at(1).y(), 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f }); // 1
                                           wfVertices.append({v.at(2).x(), v.at(2).y(), 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f }); // 2
                                       });

        wfNode.reset(new QQuickShapeWireFrameNode<SimpleWFT>);
        const QList<quint32> indices = node->uncookedIndexes();
        QSGGeometry *wfg = new QSGGeometry(QQuickShapeWireFrameNode<SimpleWFT>::attributes(),
                                           wfVertices.size(),
                                           indices.size(),
                                           QSGGeometry::UnsignedIntType);
        wfNode->setGeometry(wfg);

        wfg->setDrawingMode(QSGGeometry::DrawTriangles);
        memcpy(wfg->indexData(),
               indices.data(),
               indices.size() * wfg->sizeOfIndex());
        memcpy(wfg->vertexData(),
               wfVertices.data(),
               wfg->vertexCount() * wfg->sizeOfVertex());
    }

    if (Q_UNLIKELY(debugFlags & DebugCurves))
        node->setDebug(0.5f);

    if (node->uncookedIndexes().size() > 0) {
        node->cookGeometry();
        ret.append(node.release());
        if (wireFrame)
            ret.append(wfNode.release());
    }

    return ret;
}

QQuickShapeCurveRenderer::NodeList QQuickShapeCurveRenderer::addTriangulatingStrokerNodes(const QQuadPath &path, const QPen &pen)
{
    NodeList ret;
    const QColor &color = pen.color();

    QList<QQuickShapeWireFrameNode<StrokeWFT>::WireFrameVertex> wfVertices;

    QTriangulatingStroker stroker;
    const auto painterPath = path.toPainterPath();
    const QVectorPath &vp = qtVectorPathForPath(painterPath);
    stroker.process(vp, pen, {}, {});

    auto *node = new QSGCurveFillNode;

    auto uvForPoint = [](QVector2D v1, QVector2D v2, QVector2D p)
    {
        double divisor = v1.x() * v2.y() - v2.x() * v1.y();

        float u = (p.x() * v2.y() - p.y() * v2.x()) / divisor;
        float v = (p.y() * v1.x() - p.x() * v1.y()) / divisor;

        return QVector2D(u, v);
    };

    // Find uv coordinates for the point p, for a quadratic curve from p0 to p2 with control point p1
    // also works for a line from p0 to p2, where p1 is on the inside of the path relative to the line
    auto curveUv = [uvForPoint](QVector2D p0, QVector2D p1, QVector2D p2, QVector2D p)
    {
        QVector2D v1 = 2 * (p1 - p0);
        QVector2D v2 = p2 - v1 - p0;
        return uvForPoint(v1, v2, p - p0);
    };

    auto findPointOtherSide = [](const QVector2D &startPoint, const QVector2D &endPoint, const QVector2D &referencePoint){

        QVector2D baseLine = endPoint - startPoint;
        QVector2D insideVector = referencePoint - startPoint;
        QVector2D normal = QVector2D(-baseLine.y(), baseLine.x()); // TODO: limit size of triangle

        bool swap = QVector2D::dotProduct(insideVector, normal) < 0;

        return swap ? startPoint + normal : startPoint - normal;
    };

    static bool disableExtraTriangles = qEnvironmentVariableIntValue("QT_QUICKSHAPES_WIP_DISABLE_EXTRA_STROKE_TRIANGLES");

    auto addStrokeTriangle = [&](const QVector2D &p1, const QVector2D &p2, const QVector2D &p3){
        if (p1 == p2 || p2 == p3) {
            return;
        }

        auto uvForPoint = [&p1, &p2, &p3, curveUv](QVector2D p) {
            auto uv = curveUv(p1, p2, p3, p);
            return QVector3D(uv.x(), uv.y(), 0.0f); // Line
        };

        node->appendTriangle(p1, p2, p3, uvForPoint);


        wfVertices.append({p1.x(), p1.y(), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}); // 0
        wfVertices.append({p2.x(), p2.y(), 0.0f, 0.1f, 0.0f, 0.0f, 0.0f, 1.0f}); // 1
        wfVertices.append({p3.x(), p3.y(), 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f}); // 2

        if (!disableExtraTriangles) {
            // Add a triangle on the outer side of the line to get some more AA
            // The new point replaces p2 (currentVertex+1)
            QVector2D op = findPointOtherSide(p1, p3, p2);
            node->appendTriangle(p1, op, p3, uvForPoint);

            wfVertices.append({p1.x(), p1.y(), 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f});
            wfVertices.append({op.x(), op.y(), 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}); // replacing p2
            wfVertices.append({p3.x(), p3.y(), 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f});
        }
    };

    const int vertCount = stroker.vertexCount() / 2;
    const float *verts = stroker.vertices();
    for (int i = 0; i < vertCount - 2; ++i) {
        QVector2D p[3];
        for (int j = 0; j < 3; ++j) {
            p[j] = QVector2D(verts[(i+j)*2], verts[(i+j)*2 + 1]);
        }
        addStrokeTriangle(p[0], p[1], p[2]);
    }

    QList<quint32> indices = node->uncookedIndexes();
    if (indices.size() > 0) {
        node->setColor(color);

        node->cookGeometry();
        ret.append(node);
    }
    const bool wireFrame = debugVisualization() & DebugWireframe;
    if (wireFrame) {
        QQuickShapeWireFrameNode<StrokeWFT> *wfNode = new QQuickShapeWireFrameNode<StrokeWFT>;
        QSGGeometry *wfg = new QSGGeometry(QQuickShapeWireFrameNode<StrokeWFT>::attributes(),
                                           wfVertices.size(),
                                           indices.size(),
                                           QSGGeometry::UnsignedIntType);
        wfNode->setGeometry(wfg);

        wfg->setDrawingMode(QSGGeometry::DrawTriangles);
        memcpy(wfg->indexData(),
               indices.data(),
               indices.size() * wfg->sizeOfIndex());
        memcpy(wfg->vertexData(),
               wfVertices.data(),
               wfg->vertexCount() * wfg->sizeOfVertex());

        ret.append(wfNode);
    }

    return ret;
}

void QQuickShapeCurveRenderer::setRootNode(QSGNode *node)
{
    clearNodeReferences();
    m_rootNode = node;
}

void QQuickShapeCurveRenderer::clearNodeReferences()
{
    for (PathData &pd : m_paths) {
        pd.fillNodes.clear();
        pd.strokeNodes.clear();
    }
}

int QQuickShapeCurveRenderer::debugVisualizationFlags = QQuickShapeCurveRenderer::NoDebug;

int QQuickShapeCurveRenderer::debugVisualization()
{
    static const int envFlags = qEnvironmentVariableIntValue("QT_QUICKSHAPES_DEBUG");
    return debugVisualizationFlags | envFlags;
}

void QQuickShapeCurveRenderer::setDebugVisualization(int options)
{
    if (debugVisualizationFlags == options)
        return;
    debugVisualizationFlags = options;
}

/*! \internal
    Convert \a path to QSGCurveAbstractNodes with vertices ready to send to the GPU.
    The given \a path is assumed to be a stroke centerline: it may be continuous or dashed.
    Also create the wireframe node if enabled.
*/
QQuickShapeCurveRenderer::NodeList QQuickShapeCurveRenderer::addCurveStrokeNodes(const QQuadPath &path, const QPen &pen)
{
    NodeList ret;

    const bool debug = debugVisualization() & DebugCurves;
    auto *node = new QSGCurveStrokeNode;
    node->setDebug(0.2f * debug);
    QList<QQuickShapeWireFrameNode<StrokeWFT>::WireFrameVertex> wfVertices;

    const float penWidth = pen.widthF();

    static const int subdivisions = qEnvironmentVariable("QT_QUICKSHAPES_STROKE_SUBDIVISIONS", QStringLiteral("3")).toInt();

    const bool wireFrame = debugVisualization() & DebugWireframe;
    QSGCurveProcessor::processStroke(path,
                                     pen.miterLimit(),
                                     penWidth, pen.isCosmetic(),
                                     pen.joinStyle(),
                                     pen.capStyle(),
                                     // addStrokeTriangleCallback (see qsgcurveprocessor_p.h):
                                     [&wfVertices, &node, &wireFrame](const std::array<QVector2D, 3> &vtx,  // triangle corners
                                                          const std::array<QVector2D, 3> &ctl,  // curve control points
                                                          const std::array<QVector2D, 3> &n,    // normals
                                                          const std::array<float, 3> &ex,   // extrusions
                                                          QSGCurveStrokeNode::TriangleFlags flags)
                                    {
                                        const QVector2D &v0 = vtx.at(0);
                                        const QVector2D &v1 = vtx.at(1);
                                        const QVector2D &v2 = vtx.at(2);
                                        if (flags.testFlag(QSGCurveStrokeNode::TriangleFlag::Line))
                                            node->appendTriangle(vtx, std::array<QVector2D, 2>{ctl.at(0), ctl.at(2)}, n, ex);
                                        else
                                            node->appendTriangle(vtx, ctl, n, ex);

                                         if (Q_UNLIKELY(wireFrame)) {
                                            wfVertices.append({v0.x(), v0.y(), 1.0f, 0.0f, 0.0f, n.at(0).x(), n.at(0).y(), ex.at(0)});
                                            wfVertices.append({v1.x(), v1.y(), 0.0f, 1.0f, 0.0f, n.at(1).x(), n.at(1).y(), ex.at(1)});
                                            wfVertices.append({v2.x(), v2.y(), 0.0f, 0.0f, 1.0f, n.at(2).x(), n.at(2).y(), ex.at(2)});
                                         }
                                    },
                                    subdivisions);

    auto indexCopy = node->uncookedIndexes(); // uncookedIndexes get deleted on cooking

    node->setColor(pen.color());
    node->setStrokeWidth(penWidth);
    node->setCosmeticStroke(pen.isCosmetic());
    node->cookGeometry();
    ret.append(node);

    if (Q_UNLIKELY(wireFrame)) {
        QQuickShapeWireFrameNode<StrokeWFT> *wfNode = new QQuickShapeWireFrameNode<StrokeWFT>;

        QSGGeometry *wfg = new QSGGeometry(QQuickShapeWireFrameNode<StrokeWFT>::attributes(),
                                           wfVertices.size(),
                                           indexCopy.size(),
                                           QSGGeometry::UnsignedIntType);
        wfNode->setGeometry(wfg);
        wfNode->setCosmeticStroke(pen.isCosmetic());
        wfNode->setStrokeWidth(penWidth);

        wfg->setDrawingMode(QSGGeometry::DrawTriangles);
        memcpy(wfg->indexData(),
               indexCopy.data(),
               indexCopy.size() * wfg->sizeOfIndex());
        memcpy(wfg->vertexData(),
               wfVertices.data(),
               wfg->vertexCount() * wfg->sizeOfVertex());

        ret.append(wfNode);
    }

    return ret;
}

QT_END_NAMESPACE
