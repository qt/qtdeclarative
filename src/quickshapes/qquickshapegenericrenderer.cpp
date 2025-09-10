// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickshapegenericrenderer_p.h"
#include <QtGui/private/qtriangulator_p.h>
#include <QtGui/private/qtriangulatingstroker_p.h>
#include <rhi/qrhi.h>
#include <QSGVertexColorMaterial>
#include <QSGTextureProvider>
#include <private/qsgplaintexture_p.h>

#include <QtQuick/private/qsggradientcache_p.h>

#if QT_CONFIG(thread)
#include <QThreadPool>
#endif

QT_BEGIN_NAMESPACE

struct ColoredVertex // must match QSGGeometry::ColoredPoint2D
{
    float x, y;
    QQuickShapeGenericRenderer::Color4ub color;
    void set(float nx, float ny, QQuickShapeGenericRenderer::Color4ub ncolor)
    {
        x = nx; y = ny; color = ncolor;
    }
};

static inline QQuickShapeGenericRenderer::Color4ub colorToColor4ub(const QColor &c)
{
    float r, g, b, a;
    c.getRgbF(&r, &g, &b, &a);
    QQuickShapeGenericRenderer::Color4ub color = {
        uchar(qRound(r * a * 255)),
        uchar(qRound(g * a * 255)),
        uchar(qRound(b * a * 255)),
        uchar(qRound(a * 255))
    };
    return color;
}

QQuickShapeGenericStrokeFillNode::QQuickShapeGenericStrokeFillNode(QQuickWindow *window)
    : m_material(nullptr)
{
    setFlag(QSGNode::OwnsGeometry, true);
    setFlag(QSGNode::UsePreprocess, true);
    setGeometry(new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 0, 0));
    activateMaterial(window, MatSolidColor);
#ifdef QSG_RUNTIME_DESCRIPTION
    qsgnode_set_description(this, QLatin1String("stroke-fill"));
#endif
}

void QQuickShapeGenericStrokeFillNode::activateMaterial(QQuickWindow *window, Material m)
{
    switch (m) {
    case MatSolidColor:
        // Use vertexcolor material. Items with different colors remain batchable
        // this way, at the expense of having to provide per-vertex color values.
        m_material.reset(QQuickShapeGenericMaterialFactory::createVertexColor(window));
        break;
    case MatLinearGradient:
        m_material.reset(QQuickShapeGenericMaterialFactory::createLinearGradient(window, this));
        break;
    case MatRadialGradient:
        m_material.reset(QQuickShapeGenericMaterialFactory::createRadialGradient(window, this));
        break;
    case MatConicalGradient:
        m_material.reset(QQuickShapeGenericMaterialFactory::createConicalGradient(window, this));
        break;
    case MatTextureFill:
        m_material.reset(QQuickShapeGenericMaterialFactory::createTextureFill(window, this));
        break;
    default:
        qWarning("Unknown material %d", m);
        return;
    }

    if (material() != m_material.data())
        setMaterial(m_material.data());
}

void QQuickShapeGenericStrokeFillNode::preprocess()
{
    if (m_fillTextureProvider != nullptr) {
        if (QSGDynamicTexture *texture = qobject_cast<QSGDynamicTexture *>(m_fillTextureProvider->texture()))
            texture->updateTexture();
    }
}

void QQuickShapeGenericStrokeFillNode::handleTextureChanged()
{
    markDirty(QSGNode::DirtyMaterial);
}

void QQuickShapeGenericStrokeFillNode::handleTextureProviderDestroyed()
{
    m_fillTextureProvider = nullptr;
    markDirty(QSGNode::DirtyMaterial);
}

QQuickShapeGenericRenderer::~QQuickShapeGenericRenderer()
{
    for (ShapePathData &d : m_sp) {
        if (d.pendingFill)
            d.pendingFill->orphaned = true;
        if (d.pendingStroke)
            d.pendingStroke->orphaned = true;
    }
}

// sync, and so triangulation too, happens on the gui thread
//    - except when async is set, in which case triangulation is moved to worker threads

void QQuickShapeGenericRenderer::beginSync(int totalCount, bool *countChanged)
{
    for (int i = totalCount; i < m_sp.size(); i++) // Handle removal of paths
        setFillTextureProvider(i, nullptr); // deref window

    if (m_sp.size() != totalCount) {
        m_sp.resize(totalCount);
        m_accDirty |= DirtyList;
        *countChanged = true;
    } else {
        *countChanged = false;
    }
    for (ShapePathData &d : m_sp)
        d.syncDirty = 0;
}

void QQuickShapeGenericRenderer::setPath(int index, const QQuickPath *path)
{
    setPath(index, path ? path->path() : QPainterPath());
}

void QQuickShapeGenericRenderer::setPath(int index, const QPainterPath &path, QQuickShapePath::PathHints)
{
    ShapePathData &d(m_sp[index]);
    d.path = path;
    d.syncDirty |= DirtyFillGeom | DirtyStrokeGeom;
}

void QQuickShapeGenericRenderer::setStrokeColor(int index, const QColor &color)
{
    ShapePathData &d(m_sp[index]);
    const bool wasTransparent = d.strokeColor.a == 0;
    d.strokeColor = colorToColor4ub(color);
    const bool isTransparent = d.strokeColor.a == 0;
    d.syncDirty |= DirtyColor;
    if (wasTransparent && !isTransparent)
        d.syncDirty |= DirtyStrokeGeom;
}

void QQuickShapeGenericRenderer::setStrokeWidth(int index, qreal w)
{
    ShapePathData &d(m_sp[index]);
    d.strokeWidth = w;
    if (w > 0.0f)
        d.pen.setWidthF(w);
    d.syncDirty |= DirtyStrokeGeom;
}

void QQuickShapeGenericRenderer::setFillColor(int index, const QColor &color)
{
    ShapePathData &d(m_sp[index]);
    const bool wasTransparent = d.fillColor.a == 0;
    d.fillColor = colorToColor4ub(color);
    const bool isTransparent = d.fillColor.a == 0;
    d.syncDirty |= DirtyColor;
    if (wasTransparent && !isTransparent)
        d.syncDirty |= DirtyFillGeom;
}

void QQuickShapeGenericRenderer::setFillRule(int index, QQuickShapePath::FillRule fillRule)
{
    ShapePathData &d(m_sp[index]);
    d.fillRule = Qt::FillRule(fillRule);
    d.syncDirty |= DirtyFillGeom;
}

void QQuickShapeGenericRenderer::setJoinStyle(int index, QQuickShapePath::JoinStyle joinStyle, int miterLimit)
{
    ShapePathData &d(m_sp[index]);
    d.pen.setJoinStyle(Qt::PenJoinStyle(joinStyle));
    d.pen.setMiterLimit(miterLimit);
    d.syncDirty |= DirtyStrokeGeom;
}

void QQuickShapeGenericRenderer::setCapStyle(int index, QQuickShapePath::CapStyle capStyle)
{
    ShapePathData &d(m_sp[index]);
    d.pen.setCapStyle(Qt::PenCapStyle(capStyle));
    d.syncDirty |= DirtyStrokeGeom;
}

void QQuickShapeGenericRenderer::setStrokeStyle(int index, QQuickShapePath::StrokeStyle strokeStyle,
                                                   qreal dashOffset, const QVector<qreal> &dashPattern)
{
    ShapePathData &d(m_sp[index]);
    d.pen.setStyle(Qt::PenStyle(strokeStyle));
    if (strokeStyle == QQuickShapePath::DashLine) {
        d.pen.setDashPattern(dashPattern);
        d.pen.setDashOffset(dashOffset);
    }
    d.syncDirty |= DirtyStrokeGeom;
}

void QQuickShapeGenericRenderer::setFillGradient(int index, QQuickShapeGradient *gradient)
{
    ShapePathData &d(m_sp[index]);
    if (gradient) {
        d.fillGradient.stops = gradient->gradientStops(); // sorted
        d.fillGradient.spread = QGradient::Spread(gradient->spread());
        if (QQuickShapeLinearGradient *g  = qobject_cast<QQuickShapeLinearGradient *>(gradient)) {
            d.fillGradientActive = LinearGradient;
            d.fillGradient.a = QPointF(g->x1(), g->y1());
            d.fillGradient.b = QPointF(g->x2(), g->y2());
        } else if (QQuickShapeRadialGradient *g = qobject_cast<QQuickShapeRadialGradient *>(gradient)) {
            d.fillGradientActive = RadialGradient;
            d.fillGradient.a = QPointF(g->centerX(), g->centerY());
            d.fillGradient.b = QPointF(g->focalX(), g->focalY());
            d.fillGradient.v0 = g->centerRadius();
            d.fillGradient.v1 = g->focalRadius();
        } else if (QQuickShapeConicalGradient *g = qobject_cast<QQuickShapeConicalGradient *>(gradient)) {
            d.fillGradientActive = ConicalGradient;
            d.fillGradient.a = QPointF(g->centerX(), g->centerY());
            d.fillGradient.v0 = g->angle();
        } else {
            Q_UNREACHABLE();
        }
    } else {
        d.fillGradientActive = NoGradient;
    }
    d.syncDirty |= DirtyFillGradient;
}

void QQuickShapeGenericRenderer::setFillTextureProvider(int index, QQuickItem *textureProviderItem)
{
    ShapePathData &d(m_sp[index]);
    if ((d.fillTextureProviderItem == nullptr) != (textureProviderItem == nullptr))
        d.syncDirty |= DirtyFillGeom;
    if (d.fillTextureProviderItem != nullptr)
        QQuickItemPrivate::get(d.fillTextureProviderItem)->derefWindow();
    d.fillTextureProviderItem = textureProviderItem;
    if (d.fillTextureProviderItem != nullptr)
        QQuickItemPrivate::get(d.fillTextureProviderItem)->refWindow(m_item->window());
    d.syncDirty |= DirtyFillTexture;
}

void QQuickShapeGenericRenderer::handleSceneChange(QQuickWindow *window)
{
    for (auto &pathData : m_sp) {
        if (pathData.fillTextureProviderItem != nullptr) {
            if (window == nullptr)
                QQuickItemPrivate::get(pathData.fillTextureProviderItem)->derefWindow();
            else
                QQuickItemPrivate::get(pathData.fillTextureProviderItem)->refWindow(window);
        }
    }
}


void QQuickShapeGenericRenderer::setFillTransform(int index, const QSGTransform &transform)
{
    ShapePathData &d(m_sp[index]);
    d.fillTransform = transform;
    d.syncDirty |= DirtyFillTransform;
}

void QQuickShapeGenericRenderer::setTriangulationScale(qreal scale)
{
    // No dirty, this is called at the start of every sync. Just store the value.
    m_triangulationScale = scale;
}

void QQuickShapeFillRunnable::run()
{
    QQuickShapeGenericRenderer::triangulateFill(path, fillColor, &fillVertices, &fillIndices, &indexType,
                                                supportsElementIndexUint, triangulationScale);
    emit done(this);
}

void QQuickShapeStrokeRunnable::run()
{
    QQuickShapeGenericRenderer::triangulateStroke(path, pen, strokeColor, &strokeVertices, clipSize, triangulationScale);
    emit done(this);
}

void QQuickShapeGenericRenderer::setAsyncCallback(void (*callback)(void *), void *data)
{
    m_asyncCallback = callback;
    m_asyncCallbackData = data;
}

#if QT_CONFIG(thread)
static QThreadPool *pathWorkThreadPool = nullptr;

static void deletePathWorkThreadPool()
{
    delete pathWorkThreadPool;
    pathWorkThreadPool = nullptr;
}
#endif

void QQuickShapeGenericRenderer::endSync(bool async)
{
#if !QT_CONFIG(thread)
    // Force synchronous mode for the no-thread configuration due
    // to lack of QThreadPool.
    async = false;
#endif

    bool didKickOffAsync = false;

    for (int i = 0; i < m_sp.size(); ++i) {
        ShapePathData &d(m_sp[i]);
        if (!d.syncDirty)
            continue;

        m_accDirty |= d.syncDirty;

        // Use a shadow dirty flag in order to avoid losing state in case there are
        // multiple syncs with different dirty flags before we get to updateNode()
        // on the render thread (with the gui thread blocked). For our purposes
        // here syncDirty is still required since geometry regeneration must only
        // happen when there was an actual change in this particular sync round.
        d.effectiveDirty |= d.syncDirty;

        if (d.path.isEmpty()) {
            d.fillVertices.clear();
            d.fillIndices.clear();
            d.strokeVertices.clear();
            continue;
        }

#if QT_CONFIG(thread)
        if (async && !pathWorkThreadPool) {
            qAddPostRoutine(deletePathWorkThreadPool);
            pathWorkThreadPool = new QThreadPool;
            const int idealCount = QThread::idealThreadCount();
            pathWorkThreadPool->setMaxThreadCount(idealCount > 0 ? idealCount * 2 : 4);
        }
#endif
        auto testFeatureIndexUint = [](QQuickItem *item) -> bool {
            if (auto *w = item->window()) {
                if (auto *rhi = QQuickWindowPrivate::get(w)->rhi)
                    return rhi->isFeatureSupported(QRhi::ElementIndexUint);
            }
            return true;
        };
        static bool supportsElementIndexUint = testFeatureIndexUint(m_item);
        if ((d.syncDirty & DirtyFillGeom) && d.fillColor.a) {
            d.path.setFillRule(d.fillRule);
            if (m_api == QSGRendererInterface::Unknown)
                m_api = m_item->window()->rendererInterface()->graphicsApi();
            if (async) {
                QQuickShapeFillRunnable *r = new QQuickShapeFillRunnable;
                r->setAutoDelete(false);
                if (d.pendingFill)
                    d.pendingFill->orphaned = true;
                d.pendingFill = r;
                r->path = d.path;
                r->fillColor = d.fillColor;
                r->supportsElementIndexUint = supportsElementIndexUint;
                r->triangulationScale = m_triangulationScale;
                // Unlikely in practice but in theory m_sp could be
                // resized. Therefore, capture 'i' instead of 'd'.
                QObject::connect(r, &QQuickShapeFillRunnable::done, qApp, [this, i](QQuickShapeFillRunnable *r) {
                    // Bail out when orphaned (meaning either another run was
                    // started after this one, or the renderer got destroyed).
                    if (!r->orphaned && i < m_sp.size()) {
                        ShapePathData &d(m_sp[i]);
                        d.fillVertices = r->fillVertices;
                        d.fillIndices = r->fillIndices;
                        d.indexType = r->indexType;
                        d.pendingFill = nullptr;
                        d.effectiveDirty |= DirtyFillGeom;
                        maybeUpdateAsyncItem();
                    }
                    r->deleteLater();
                });
                didKickOffAsync = true;
#if QT_CONFIG(thread)
                // qtVectorPathForPath() initializes a unique_ptr without locking.
                // Do that before starting the threads as otherwise we get a race condition.
                qtVectorPathForPath(r->path);
                pathWorkThreadPool->start(r);
#endif
            } else {
                triangulateFill(d.path, d.fillColor, &d.fillVertices, &d.fillIndices, &d.indexType,
                                supportsElementIndexUint,
                                m_triangulationScale);
            }
        }

        if ((d.syncDirty & DirtyStrokeGeom) && d.strokeWidth > 0.0f && d.strokeColor.a) {
            if (async) {
                QQuickShapeStrokeRunnable *r = new QQuickShapeStrokeRunnable;
                r->setAutoDelete(false);
                if (d.pendingStroke)
                    d.pendingStroke->orphaned = true;
                d.pendingStroke = r;
                r->path = d.path;
                r->pen = d.pen;
                r->strokeColor = d.strokeColor;
                r->clipSize = QSize(m_item->width(), m_item->height());
                r->triangulationScale = m_triangulationScale;
                QObject::connect(r, &QQuickShapeStrokeRunnable::done, qApp, [this, i](QQuickShapeStrokeRunnable *r) {
                    if (!r->orphaned && i < m_sp.size()) {
                        ShapePathData &d(m_sp[i]);
                        d.strokeVertices = r->strokeVertices;
                        d.pendingStroke = nullptr;
                        d.effectiveDirty |= DirtyStrokeGeom;
                        maybeUpdateAsyncItem();
                    }
                    r->deleteLater();
                });
                didKickOffAsync = true;
#if QT_CONFIG(thread)
                // qtVectorPathForPath() initializes a unique_ptr without locking.
                // Do that before starting the threads as otherwise we get a race condition.
                qtVectorPathForPath(r->path);
                pathWorkThreadPool->start(r);
#endif
            } else {
                triangulateStroke(d.path, d.pen, d.strokeColor, &d.strokeVertices,
                                  QSize(m_item->width(), m_item->height()), m_triangulationScale);
            }
        }
    }

    if (!didKickOffAsync && async && m_asyncCallback)
        m_asyncCallback(m_asyncCallbackData);
}

void QQuickShapeGenericRenderer::maybeUpdateAsyncItem()
{
    for (const ShapePathData &d : std::as_const(m_sp)) {
        if (d.pendingFill || d.pendingStroke)
            return;
    }
    m_accDirty |= DirtyFillGeom | DirtyStrokeGeom;
    m_item->update();
    if (m_asyncCallback)
        m_asyncCallback(m_asyncCallbackData);
}

// the stroke/fill triangulation functions may be invoked either on the gui
// thread or some worker thread and must thus be self-contained.
void QQuickShapeGenericRenderer::triangulateFill(const QPainterPath &path,
                                                 const Color4ub &fillColor,
                                                 VertexContainerType *fillVertices,
                                                 IndexContainerType *fillIndices,
                                                 QSGGeometry::Type *indexType,
                                                 bool supportsElementIndexUint,
                                                 qreal triangulationScale)
{
    const QVectorPath &vp = qtVectorPathForPath(path);

    QTriangleSet ts = qTriangulate(vp, QTransform::fromScale(triangulationScale, triangulationScale), 1, supportsElementIndexUint);
    const int vertexCount = ts.vertices.size() / 2; // just a qreal vector with x,y hence the / 2
    fillVertices->resize(vertexCount);
    ColoredVertex *vdst = reinterpret_cast<ColoredVertex *>(fillVertices->data());
    const qreal *vsrc = ts.vertices.constData();
    for (int i = 0; i < vertexCount; ++i)
        vdst[i].set(vsrc[i * 2] / triangulationScale, vsrc[i * 2 + 1] / triangulationScale, fillColor);

    size_t indexByteSize;
    if (ts.indices.type() == QVertexIndexVector::UnsignedShort) {
        *indexType = QSGGeometry::UnsignedShortType;
        // fillIndices is still QVector<quint32>. Just resize to N/2 and pack
        // the N quint16s into it.
        fillIndices->resize(ts.indices.size() / 2);
        indexByteSize = ts.indices.size() * sizeof(quint16);
    } else {
        *indexType = QSGGeometry::UnsignedIntType;
        fillIndices->resize(ts.indices.size());
        indexByteSize = ts.indices.size() * sizeof(quint32);
    }
    memcpy(fillIndices->data(), ts.indices.data(), indexByteSize);
}

void QQuickShapeGenericRenderer::triangulateStroke(const QPainterPath &path,
                                                   const QPen &pen,
                                                   const Color4ub &strokeColor,
                                                   VertexContainerType *strokeVertices,
                                                   const QSize &clipSize,
                                                   qreal triangulationScale)
{
    const QVectorPath &vp = qtVectorPathForPath(path);
    const QRectF clip(QPointF(0, 0), clipSize);
    const qreal inverseScale = 1.0 / triangulationScale;

    QTriangulatingStroker stroker;
    stroker.setInvScale(inverseScale);

    if (pen.style() == Qt::SolidLine) {
        stroker.process(vp, pen, clip, {});
    } else {
        QDashedStrokeProcessor dashStroker;
        dashStroker.setInvScale(inverseScale);
        dashStroker.process(vp, pen, clip, {});
        QVectorPath dashStroke(dashStroker.points(), dashStroker.elementCount(),
                               dashStroker.elementTypes(), 0);
        stroker.process(dashStroke, pen, clip, {});
    }

    if (!stroker.vertexCount()) {
        strokeVertices->clear();
        return;
    }

    const int vertexCount = stroker.vertexCount() / 2; // just a float vector with x,y hence the / 2
    strokeVertices->resize(vertexCount);
    ColoredVertex *vdst = reinterpret_cast<ColoredVertex *>(strokeVertices->data());
    const float *vsrc = stroker.vertices();
    for (int i = 0; i < vertexCount; ++i)
        vdst[i].set(vsrc[i * 2], vsrc[i * 2 + 1], strokeColor);
}

void QQuickShapeGenericRenderer::setRootNode(QQuickShapeGenericNode *node)
{
    if (m_rootNode != node) {
        m_rootNode = node;
        m_accDirty |= DirtyList;
    }
}

// on the render thread with gui blocked
void QQuickShapeGenericRenderer::updateNode()
{
    if (!m_rootNode || !m_accDirty)
        return;

//                     [   m_rootNode   ]
//                     /       /        /
// #0          [  fill  ] [  stroke  ] [   next   ]
//                                    /     /      |
// #1                       [  fill  ] [  stroke  ] [   next   ]
//                                                 /      /     |
// #2                                     [  fill  ] [ stroke ] [  next  ]
//                                                                 ...
// ...

    QQuickShapeGenericNode **nodePtr = &m_rootNode;
    QQuickShapeGenericNode *prevNode = nullptr;

    for (ShapePathData &d : m_sp) {
        if (!*nodePtr) {
            Q_ASSERT(prevNode);
            *nodePtr = new QQuickShapeGenericNode;
            prevNode->m_next = *nodePtr;
            prevNode->appendChildNode(*nodePtr);
        }

        QQuickShapeGenericNode *node = *nodePtr;

        if (m_accDirty & DirtyList)
            d.effectiveDirty |= DirtyFillGeom | DirtyStrokeGeom | DirtyColor | DirtyFillGradient | DirtyFillTransform | DirtyFillTexture;

        if (!d.effectiveDirty) {
            prevNode = node;
            nodePtr = &node->m_next;
            continue;
        }

        if (d.fillColor.a == 0) {
            delete node->m_fillNode;
            node->m_fillNode = nullptr;
        } else if (!node->m_fillNode) {
            node->m_fillNode = new QQuickShapeGenericStrokeFillNode(m_item->window());
            if (node->m_strokeNode)
                node->removeChildNode(node->m_strokeNode);
            node->appendChildNode(node->m_fillNode);
            if (node->m_strokeNode)
                node->appendChildNode(node->m_strokeNode);
            d.effectiveDirty |= DirtyFillGeom;
        }

        if (d.strokeWidth <= 0.0f || d.strokeColor.a == 0) {
            delete node->m_strokeNode;
            node->m_strokeNode = nullptr;
        } else if (!node->m_strokeNode) {
            node->m_strokeNode = new QQuickShapeGenericStrokeFillNode(m_item->window());
            node->appendChildNode(node->m_strokeNode);
            d.effectiveDirty |= DirtyStrokeGeom;
        }

        updateFillNode(&d, node);
        updateStrokeNode(&d, node);

        d.effectiveDirty = 0;

        prevNode = node;
        nodePtr = &node->m_next;
    }

    if (*nodePtr && prevNode) {
        prevNode->removeChildNode(*nodePtr);
        delete *nodePtr;
        *nodePtr = nullptr;
    }

    if (m_sp.isEmpty()) {
        delete m_rootNode->m_fillNode;
        m_rootNode->m_fillNode = nullptr;
        delete m_rootNode->m_strokeNode;
        m_rootNode->m_strokeNode = nullptr;
        delete m_rootNode->m_next;
        m_rootNode->m_next = nullptr;
    }

    m_accDirty = 0;
}

void QQuickShapeGenericRenderer::updateShadowDataInNode(ShapePathData *d, QQuickShapeGenericStrokeFillNode *n)
{
    if (d->fillGradientActive) {
        if (d->effectiveDirty & DirtyFillGradient)
            n->m_fillGradient = d->fillGradient;
    }
    if (d->effectiveDirty & DirtyFillTexture) {
        bool needsUpdate = d->fillTextureProviderItem == nullptr && n->m_fillTextureProvider != nullptr;
        if (!needsUpdate
            && d->fillTextureProviderItem != nullptr
            && n->m_fillTextureProvider != d->fillTextureProviderItem->textureProvider()) {
            needsUpdate = true;
        }

        if (needsUpdate) {
            if (n->m_fillTextureProvider != nullptr) {
                QObject::disconnect(n->m_fillTextureProvider, &QSGTextureProvider::textureChanged,
                                    n, &QQuickShapeGenericStrokeFillNode::handleTextureChanged);
                QObject::disconnect(n->m_fillTextureProvider, &QSGTextureProvider::destroyed,
                                    n, &QQuickShapeGenericStrokeFillNode::handleTextureProviderDestroyed);
            }

            n->m_fillTextureProvider = d->fillTextureProviderItem == nullptr
                                           ? nullptr
                                           : d->fillTextureProviderItem->textureProvider();

            if (n->m_fillTextureProvider != nullptr) {
                QObject::connect(n->m_fillTextureProvider, &QSGTextureProvider::textureChanged,
                                 n, &QQuickShapeGenericStrokeFillNode::handleTextureChanged);
                QObject::connect(n->m_fillTextureProvider, &QSGTextureProvider::destroyed,
                                 n, &QQuickShapeGenericStrokeFillNode::handleTextureProviderDestroyed);
            }
        }
    }
    if (d->effectiveDirty & DirtyFillTransform)
        n->m_fillTransform = d->fillTransform;
}

void QQuickShapeGenericRenderer::updateFillNode(ShapePathData *d, QQuickShapeGenericNode *node)
{
    if (!node->m_fillNode)
        return;
    if (!(d->effectiveDirty & (DirtyFillGeom | DirtyColor | DirtyFillGradient | DirtyFillTransform | DirtyFillTexture)))
        return;

    // Make a copy of the data that will be accessed by the material on
    // the render thread. This must be done even when we bail out below.
    QQuickShapeGenericStrokeFillNode *n = node->m_fillNode;
    updateShadowDataInNode(d, n);

    QSGGeometry *g = n->geometry();
    if (d->fillVertices.isEmpty()) {
        if (g->vertexCount() || g->indexCount()) {
            g->allocate(0, 0);
            n->markDirty(QSGNode::DirtyGeometry);
        }
        return;
    }

    if (d->fillGradientActive) {
        QQuickShapeGenericStrokeFillNode::Material gradMat;
        switch (d->fillGradientActive) {
        case LinearGradient:
            gradMat = QQuickShapeGenericStrokeFillNode::MatLinearGradient;
            break;
        case RadialGradient:
            gradMat = QQuickShapeGenericStrokeFillNode::MatRadialGradient;
            break;
        case ConicalGradient:
            gradMat = QQuickShapeGenericStrokeFillNode::MatConicalGradient;
            break;
        default:
            Q_UNREACHABLE_RETURN();
        }
        n->activateMaterial(m_item->window(), gradMat);
        if (d->effectiveDirty & (DirtyFillGradient | DirtyFillTransform)) {
            // Gradients are implemented via a texture-based material.
            n->markDirty(QSGNode::DirtyMaterial);
            // stop here if only the gradient or filltransform changed; no need to touch the geometry
            if (!(d->effectiveDirty & DirtyFillGeom))
                return;
        }
    } else if (d->fillTextureProviderItem != nullptr) {
        n->activateMaterial(m_item->window(), QQuickShapeGenericStrokeFillNode::MatTextureFill);
        if (d->effectiveDirty & DirtyFillTexture)
            n->markDirty(QSGNode::DirtyMaterial);
    } else {
        n->activateMaterial(m_item->window(), QQuickShapeGenericStrokeFillNode::MatSolidColor);
        // fast path for updating only color values when no change in vertex positions
        if ((d->effectiveDirty & DirtyColor) && !(d->effectiveDirty & DirtyFillGeom) && d->fillTextureProviderItem == nullptr) {
            ColoredVertex *vdst = reinterpret_cast<ColoredVertex *>(g->vertexData());
            for (int i = 0; i < g->vertexCount(); ++i)
                vdst[i].set(vdst[i].x, vdst[i].y, d->fillColor);
            n->markDirty(QSGNode::DirtyGeometry);
            return;
        }
    }

    const int indexCount = d->indexType == QSGGeometry::UnsignedShortType
            ? d->fillIndices.size() * 2 : d->fillIndices.size();
    if (g->indexType() != d->indexType) {
        g = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(),
                            d->fillVertices.size(), indexCount, d->indexType);
        n->setGeometry(g);
    } else {
        g->allocate(d->fillVertices.size(), indexCount);
    }
    g->setDrawingMode(QSGGeometry::DrawTriangles);

    memcpy(g->vertexData(), d->fillVertices.constData(), g->vertexCount() * g->sizeOfVertex());
    memcpy(g->indexData(), d->fillIndices.constData(), g->indexCount() * g->sizeOfIndex());

    n->markDirty(QSGNode::DirtyGeometry);
}

void QQuickShapeGenericRenderer::updateStrokeNode(ShapePathData *d, QQuickShapeGenericNode *node)
{
    if (!node->m_strokeNode)
        return;
    if (!(d->effectiveDirty & (DirtyStrokeGeom | DirtyColor)))
        return;

    QQuickShapeGenericStrokeFillNode *n = node->m_strokeNode;
    QSGGeometry *g = n->geometry();
    if (d->strokeVertices.isEmpty()) {
        if (g->vertexCount() || g->indexCount()) {
            g->allocate(0, 0);
            n->markDirty(QSGNode::DirtyGeometry);
        }
        return;
    }

    n->markDirty(QSGNode::DirtyGeometry);

    // Async loading runs update once, bails out above, then updates again once
    // ready. Set the material dirty then. This is in-line with fill where the
    // first activateMaterial() achieves the same.
    if (!g->vertexCount())
        n->markDirty(QSGNode::DirtyMaterial);

    if ((d->effectiveDirty & DirtyColor) && !(d->effectiveDirty & DirtyStrokeGeom)) {
        ColoredVertex *vdst = reinterpret_cast<ColoredVertex *>(g->vertexData());
        for (int i = 0; i < g->vertexCount(); ++i)
            vdst[i].set(vdst[i].x, vdst[i].y, d->strokeColor);
        return;
    }

    g->allocate(d->strokeVertices.size(), 0);
    g->setDrawingMode(QSGGeometry::DrawTriangleStrip);
    memcpy(g->vertexData(), d->strokeVertices.constData(), g->vertexCount() * g->sizeOfVertex());
}

QSGMaterial *QQuickShapeGenericMaterialFactory::createVertexColor(QQuickWindow *window)
{
    QSGRendererInterface::GraphicsApi api = window->rendererInterface()->graphicsApi();

    if (api == QSGRendererInterface::OpenGL || QSGRendererInterface::isApiRhiBased(api))
        return new QSGVertexColorMaterial;

    qWarning("Vertex-color material: Unsupported graphics API %d", api);
    return nullptr;
}

QSGMaterial *QQuickShapeGenericMaterialFactory::createLinearGradient(QQuickWindow *window,
                                                                     QQuickShapeGenericStrokeFillNode *node)
{
    QSGRendererInterface::GraphicsApi api = window->rendererInterface()->graphicsApi();

    if (api == QSGRendererInterface::OpenGL || QSGRendererInterface::isApiRhiBased(api))
        return new QQuickShapeLinearGradientMaterial(node);

    qWarning("Linear gradient material: Unsupported graphics API %d", api);
    return nullptr;
}

QSGMaterial *QQuickShapeGenericMaterialFactory::createRadialGradient(QQuickWindow *window,
                                                                     QQuickShapeGenericStrokeFillNode *node)
{
    QSGRendererInterface::GraphicsApi api = window->rendererInterface()->graphicsApi();

    if (api == QSGRendererInterface::OpenGL || QSGRendererInterface::isApiRhiBased(api))
        return new QQuickShapeRadialGradientMaterial(node);

    qWarning("Radial gradient material: Unsupported graphics API %d", api);
    return nullptr;
}

QSGMaterial *QQuickShapeGenericMaterialFactory::createConicalGradient(QQuickWindow *window,
                                                                      QQuickShapeGenericStrokeFillNode *node)
{
    QSGRendererInterface::GraphicsApi api = window->rendererInterface()->graphicsApi();

    if (api == QSGRendererInterface::OpenGL || QSGRendererInterface::isApiRhiBased(api))
        return new QQuickShapeConicalGradientMaterial(node);

    qWarning("Conical gradient material: Unsupported graphics API %d", api);
    return nullptr;
}

QSGMaterial *QQuickShapeGenericMaterialFactory::createTextureFill(QQuickWindow *window,
                                                                  QQuickShapeGenericStrokeFillNode *node)
{
    QSGRendererInterface::GraphicsApi api = window->rendererInterface()->graphicsApi();

    if (api == QSGRendererInterface::OpenGL || QSGRendererInterface::isApiRhiBased(api))
        return new QQuickShapeTextureFillMaterial(node);

    qWarning("Texture fill material: Unsupported graphics API %d", api);
    return nullptr;
}

QQuickShapeLinearGradientRhiShader::QQuickShapeLinearGradientRhiShader(int viewCount)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/shapes/shaders_ng/lineargradient.vert.qsb"), viewCount);
    setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/shapes/shaders_ng/lineargradient.frag.qsb"), viewCount);
}

bool QQuickShapeLinearGradientRhiShader::updateUniformData(RenderState &state,
                                                           QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_ASSERT(oldMaterial == nullptr || newMaterial->type() == oldMaterial->type());
    QQuickShapeLinearGradientMaterial *m = static_cast<QQuickShapeLinearGradientMaterial *>(newMaterial);
    bool changed = false;
    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 84 + 64);
    const int shaderMatrixCount = newMaterial->viewCount();
    const int matrixCount = qMin(state.projectionMatrixCount(), shaderMatrixCount);

    if (state.isMatrixDirty()) {
        for (int viewIndex = 0; viewIndex < matrixCount; ++viewIndex) {
            const QMatrix4x4 m = state.combinedMatrix();
            memcpy(buf->data() + 64 * viewIndex, m.constData(), 64);
            changed = true;
        }
    }

    QQuickShapeGenericStrokeFillNode *node = m->node();

    if (!oldMaterial || m_fillTransform != node->m_fillTransform) {
        memcpy(buf->data() + 64 * shaderMatrixCount, node->m_fillTransform.invertedData(), 64);
        m_fillTransform = node->m_fillTransform;
        changed = true;
    }

    if (!oldMaterial || m_gradA.x() != node->m_fillGradient.a.x() || m_gradA.y() != node->m_fillGradient.a.y()) {
        m_gradA = QVector2D(node->m_fillGradient.a.x(), node->m_fillGradient.a.y());
        Q_ASSERT(sizeof(m_gradA) == 8);
        memcpy(buf->data() + 64 * shaderMatrixCount + 64, &m_gradA, 8);
        changed = true;
    }

    if (!oldMaterial || m_gradB.x() != node->m_fillGradient.b.x() || m_gradB.y() != node->m_fillGradient.b.y()) {
        m_gradB = QVector2D(node->m_fillGradient.b.x(), node->m_fillGradient.b.y());
        memcpy(buf->data() + 64 * shaderMatrixCount + 64 + 8, &m_gradB, 8);
        changed = true;
    }

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 64 * shaderMatrixCount + 64 + 8 + 8, &opacity, 4);
        changed = true;
    }

    return changed;
}

void QQuickShapeLinearGradientRhiShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                            QSGMaterial *newMaterial, QSGMaterial *)
{
    if (binding != 1)
        return;

    QQuickShapeLinearGradientMaterial *m = static_cast<QQuickShapeLinearGradientMaterial *>(newMaterial);
    QQuickShapeGenericStrokeFillNode *node = m->node();
    const QSGGradientCacheKey cacheKey(node->m_fillGradient.stops, QGradient::Spread(node->m_fillGradient.spread));
    QSGTexture *t = QSGGradientCache::cacheForRhi(state.rhi())->get(cacheKey);
    t->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
    *texture = t;
}

QSGMaterialType *QQuickShapeLinearGradientMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

int QQuickShapeLinearGradientMaterial::compare(const QSGMaterial *other) const
{
    Q_ASSERT(other && type() == other->type());
    const QQuickShapeLinearGradientMaterial *m = static_cast<const QQuickShapeLinearGradientMaterial *>(other);

    QQuickShapeGenericStrokeFillNode *a = node();
    QQuickShapeGenericStrokeFillNode *b = m->node();
    Q_ASSERT(a && b);
    if (a == b)
        return 0;

    const QSGGradientCache::GradientDesc *ga = &a->m_fillGradient;
    const QSGGradientCache::GradientDesc *gb = &b->m_fillGradient;

    if (int d = ga->spread - gb->spread)
        return d;

    if (int d = ga->a.x() - gb->a.x())
        return d;
    if (int d = ga->a.y() - gb->a.y())
        return d;
    if (int d = ga->b.x() - gb->b.x())
        return d;
    if (int d = ga->b.y() - gb->b.y())
        return d;

    if (int d = ga->stops.size() - gb->stops.size())
        return d;

    for (int i = 0; i < ga->stops.size(); ++i) {
        if (int d = ga->stops[i].first - gb->stops[i].first)
            return d;
        if (int d = ga->stops[i].second.rgba() - gb->stops[i].second.rgba())
            return d;
    }

    if (int d = a->m_fillTransform.compareTo(b->m_fillTransform))
        return d;

    return 0;
}

QSGMaterialShader *QQuickShapeLinearGradientMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    return new QQuickShapeLinearGradientRhiShader(viewCount());
}

QQuickShapeRadialGradientRhiShader::QQuickShapeRadialGradientRhiShader(int viewCount)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/shapes/shaders_ng/radialgradient.vert.qsb"), viewCount);
    setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/shapes/shaders_ng/radialgradient.frag.qsb"), viewCount);
}

bool QQuickShapeRadialGradientRhiShader::updateUniformData(RenderState &state,
                                                           QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_ASSERT(oldMaterial == nullptr || newMaterial->type() == oldMaterial->type());
    QQuickShapeRadialGradientMaterial *m = static_cast<QQuickShapeRadialGradientMaterial *>(newMaterial);
    bool changed = false;
    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 92 + 64);
    const int shaderMatrixCount = newMaterial->viewCount();
    const int matrixCount = qMin(state.projectionMatrixCount(), shaderMatrixCount);

    if (state.isMatrixDirty()) {
        for (int viewIndex = 0; viewIndex < matrixCount; ++viewIndex) {
            const QMatrix4x4 m = state.combinedMatrix();
            memcpy(buf->data() + 64 * viewIndex, m.constData(), 64);
            changed = true;
        }
    }

    QQuickShapeGenericStrokeFillNode *node = m->node();

    if (!oldMaterial || m_fillTransform != node->m_fillTransform) {
        memcpy(buf->data() + 64 * shaderMatrixCount, node->m_fillTransform.invertedData(), 64);
        m_fillTransform = node->m_fillTransform;
        changed = true;
    }

    const QPointF centerPoint = node->m_fillGradient.a;
    const QPointF focalPoint = node->m_fillGradient.b;
    const QPointF focalToCenter = centerPoint - focalPoint;
    const float centerRadius = node->m_fillGradient.v0;
    const float focalRadius = node->m_fillGradient.v1;

    if (!oldMaterial || m_focalPoint.x() != focalPoint.x() || m_focalPoint.y() != focalPoint.y()) {
        m_focalPoint = QVector2D(focalPoint.x(), focalPoint.y());
        Q_ASSERT(sizeof(m_focalPoint) == 8);
        memcpy(buf->data() + 64 * shaderMatrixCount + 64, &m_focalPoint, 8);
        changed = true;
    }

    if (!oldMaterial || m_focalToCenter.x() != focalToCenter.x() || m_focalToCenter.y() != focalToCenter.y()) {
        m_focalToCenter = QVector2D(focalToCenter.x(), focalToCenter.y());
        Q_ASSERT(sizeof(m_focalToCenter) == 8);
        memcpy(buf->data() + 64 * shaderMatrixCount + 64 + 8, &m_focalToCenter, 8);
        changed = true;
    }

    if (!oldMaterial || m_centerRadius != centerRadius) {
        m_centerRadius = centerRadius;
        memcpy(buf->data() + 64 * shaderMatrixCount + 64 + 8 + 8, &m_centerRadius, 4);
        changed = true;
    }

    if (!oldMaterial || m_focalRadius != focalRadius) {
        m_focalRadius = focalRadius;
        memcpy(buf->data() + 64 * shaderMatrixCount + 64 + 8 + 8 + 4, &m_focalRadius, 4);
        changed = true;
    }

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 64 * shaderMatrixCount + 64 + 8 + 8 + 4 + 4, &opacity, 4);
        changed = true;
    }

    return changed;
}

void QQuickShapeRadialGradientRhiShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                            QSGMaterial *newMaterial, QSGMaterial *)
{
    if (binding != 1)
        return;

    QQuickShapeRadialGradientMaterial *m = static_cast<QQuickShapeRadialGradientMaterial *>(newMaterial);
    QQuickShapeGenericStrokeFillNode *node = m->node();
    const QSGGradientCacheKey cacheKey(node->m_fillGradient.stops, QGradient::Spread(node->m_fillGradient.spread));
    QSGTexture *t = QSGGradientCache::cacheForRhi(state.rhi())->get(cacheKey);
    t->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
    *texture = t;
}

QSGMaterialType *QQuickShapeRadialGradientMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

int QQuickShapeRadialGradientMaterial::compare(const QSGMaterial *other) const
{
    Q_ASSERT(other && type() == other->type());
    const QQuickShapeRadialGradientMaterial *m = static_cast<const QQuickShapeRadialGradientMaterial *>(other);

    QQuickShapeGenericStrokeFillNode *a = node();
    QQuickShapeGenericStrokeFillNode *b = m->node();
    Q_ASSERT(a && b);
    if (a == b)
        return 0;

    const QSGGradientCache::GradientDesc *ga = &a->m_fillGradient;
    const QSGGradientCache::GradientDesc *gb = &b->m_fillGradient;

    if (int d = ga->spread - gb->spread)
        return d;

    if (int d = ga->a.x() - gb->a.x())
        return d;
    if (int d = ga->a.y() - gb->a.y())
        return d;
    if (int d = ga->b.x() - gb->b.x())
        return d;
    if (int d = ga->b.y() - gb->b.y())
        return d;

    if (int d = ga->v0 - gb->v0)
        return d;
    if (int d = ga->v1 - gb->v1)
        return d;

    if (int d = ga->stops.size() - gb->stops.size())
        return d;

    for (int i = 0; i < ga->stops.size(); ++i) {
        if (int d = ga->stops[i].first - gb->stops[i].first)
            return d;
        if (int d = ga->stops[i].second.rgba() - gb->stops[i].second.rgba())
            return d;
    }

    if (int d = a->m_fillTransform.compareTo(b->m_fillTransform))
        return d;

    return 0;
}

QSGMaterialShader *QQuickShapeRadialGradientMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    return new QQuickShapeRadialGradientRhiShader(viewCount());
}

QQuickShapeConicalGradientRhiShader::QQuickShapeConicalGradientRhiShader(int viewCount)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/shapes/shaders_ng/conicalgradient.vert.qsb"), viewCount);
    setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/shapes/shaders_ng/conicalgradient.frag.qsb"), viewCount);
}

bool QQuickShapeConicalGradientRhiShader::updateUniformData(RenderState &state,
                                                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_ASSERT(oldMaterial == nullptr || newMaterial->type() == oldMaterial->type());
    QQuickShapeConicalGradientMaterial *m = static_cast<QQuickShapeConicalGradientMaterial *>(newMaterial);
    bool changed = false;
    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 80 + 64);
    const int shaderMatrixCount = newMaterial->viewCount();
    const int matrixCount = qMin(state.projectionMatrixCount(), shaderMatrixCount);

    if (state.isMatrixDirty()) {
        for (int viewIndex = 0; viewIndex < matrixCount; ++viewIndex) {
            const QMatrix4x4 m = state.combinedMatrix();
            memcpy(buf->data() + 64 * viewIndex, m.constData(), 64);
            changed = true;
        }
    }

    QQuickShapeGenericStrokeFillNode *node = m->node();

    if (!oldMaterial || m_fillTransform != node->m_fillTransform) {
        memcpy(buf->data() + 64 * shaderMatrixCount, node->m_fillTransform.invertedData(), 64);
        m_fillTransform = node->m_fillTransform;
        changed = true;
    }

    const QPointF centerPoint = node->m_fillGradient.a;
    const float angle = -qDegreesToRadians(node->m_fillGradient.v0);

    if (!oldMaterial || m_centerPoint.x() != centerPoint.x() || m_centerPoint.y() != centerPoint.y()) {
        m_centerPoint = QVector2D(centerPoint.x(), centerPoint.y());
        Q_ASSERT(sizeof(m_centerPoint) == 8);
        memcpy(buf->data() + 64 * shaderMatrixCount + 64, &m_centerPoint, 8);
        changed = true;
    }

    if (!oldMaterial || m_angle != angle) {
        m_angle = angle;
        memcpy(buf->data() + 64 * shaderMatrixCount + 64 + 8, &m_angle, 4);
        changed = true;
    }

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 64 * shaderMatrixCount + 64 + 8 + 4, &opacity, 4);
        changed = true;
    }

    return changed;
}

void QQuickShapeConicalGradientRhiShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                             QSGMaterial *newMaterial, QSGMaterial *)
{
    if (binding != 1)
        return;

    QQuickShapeConicalGradientMaterial *m = static_cast<QQuickShapeConicalGradientMaterial *>(newMaterial);
    QQuickShapeGenericStrokeFillNode *node = m->node();
    const QSGGradientCacheKey cacheKey(node->m_fillGradient.stops, QGradient::Spread(node->m_fillGradient.spread));
    QSGTexture *t = QSGGradientCache::cacheForRhi(state.rhi())->get(cacheKey);
    t->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
    *texture = t;
}

QSGMaterialType *QQuickShapeConicalGradientMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

int QQuickShapeConicalGradientMaterial::compare(const QSGMaterial *other) const
{
    Q_ASSERT(other && type() == other->type());
    const QQuickShapeConicalGradientMaterial *m = static_cast<const QQuickShapeConicalGradientMaterial *>(other);

    QQuickShapeGenericStrokeFillNode *a = node();
    QQuickShapeGenericStrokeFillNode *b = m->node();
    Q_ASSERT(a && b);
    if (a == b)
        return 0;

    const QSGGradientCache::GradientDesc *ga = &a->m_fillGradient;
    const QSGGradientCache::GradientDesc *gb = &b->m_fillGradient;

    if (int d = ga->a.x() - gb->a.x())
        return d;
    if (int d = ga->a.y() - gb->a.y())
        return d;

    if (int d = ga->v0 - gb->v0)
        return d;

    if (int d = ga->stops.size() - gb->stops.size())
        return d;

    for (int i = 0; i < ga->stops.size(); ++i) {
        if (int d = ga->stops[i].first - gb->stops[i].first)
            return d;
        if (int d = ga->stops[i].second.rgba() - gb->stops[i].second.rgba())
            return d;
    }

    if (int d = a->m_fillTransform.compareTo(b->m_fillTransform))
        return d;

    return 0;
}

QSGMaterialShader *QQuickShapeConicalGradientMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    return new QQuickShapeConicalGradientRhiShader(viewCount());
}

QQuickShapeTextureFillRhiShader::QQuickShapeTextureFillRhiShader(int viewCount)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/shapes/shaders_ng/texturefill.vert.qsb"), viewCount);
    setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/shapes/shaders_ng/texturefill.frag.qsb"), viewCount);
}

bool QQuickShapeTextureFillRhiShader::updateUniformData(RenderState &state,
                                                        QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_ASSERT(oldMaterial == nullptr || newMaterial->type() == oldMaterial->type());
    QQuickShapeTextureFillMaterial *m = static_cast<QQuickShapeTextureFillMaterial *>(newMaterial);
    bool changed = false;
    QByteArray *buf = state.uniformData();
    const int shaderMatrixCount = newMaterial->viewCount();
    const int matrixCount = qMin(state.projectionMatrixCount(), shaderMatrixCount);
    Q_ASSERT(buf->size() >= 64 * shaderMatrixCount + 64 + 8 + 4);

    if (state.isMatrixDirty()) {
        for (int viewIndex = 0; viewIndex < matrixCount; ++viewIndex) {
            const QMatrix4x4 m = state.combinedMatrix();
            memcpy(buf->data() + 64 * viewIndex, m.constData(), 64);
            changed = true;
        }
    }

    QQuickShapeGenericStrokeFillNode *node = m->node();

    if (!oldMaterial || m_fillTransform != node->m_fillTransform) {
        memcpy(buf->data() + 64 * shaderMatrixCount, node->m_fillTransform.invertedData(), 64);
        m_fillTransform = node->m_fillTransform;
        changed = true;
    }

    const QSizeF boundsSize = node->m_fillTextureProvider != nullptr && node->m_fillTextureProvider->texture() != nullptr
        ? node->m_fillTextureProvider->texture()->textureSize()
        : QSizeF(0, 0);


    const QVector2D boundsVector(boundsSize.width() / state.devicePixelRatio(),
                                 boundsSize.height() / state.devicePixelRatio());
    if (!oldMaterial || m_boundsSize != boundsVector) {
        m_boundsSize = boundsVector;
        Q_ASSERT(sizeof(m_boundsSize) == 8);
        memcpy(buf->data() + 64 * shaderMatrixCount + 64, &m_boundsSize, 8);
        changed = true;
    }

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 64 * shaderMatrixCount + 64 + 8, &opacity, 4);
        changed = true;
    }

    return changed;
}

void QQuickShapeTextureFillRhiShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                         QSGMaterial *newMaterial, QSGMaterial *)
{
    if (binding != 1)
        return;

    QQuickShapeTextureFillMaterial *m = static_cast<QQuickShapeTextureFillMaterial *>(newMaterial);
    QQuickShapeGenericStrokeFillNode *node = m->node();
    if (node->m_fillTextureProvider != nullptr) {
        QSGTexture *providedTexture = node->m_fillTextureProvider->texture();
        if (providedTexture != nullptr) {
            if (providedTexture->isAtlasTexture()) {
                // Create a non-atlas copy to make texture coordinate wrapping work. This
                // texture copy is owned by the QSGTexture so memory is managed with the original
                // texture provider.
                QSGTexture *newTexture = providedTexture->removedFromAtlas(state.resourceUpdateBatch());
                if (newTexture != nullptr)
                    providedTexture = newTexture;
            }

            providedTexture->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
            *texture = providedTexture;
            return;
        }
    }

    if (m->dummyTexture() == nullptr) {
        QSGPlainTexture *dummyTexture = new QSGPlainTexture;
        dummyTexture->setFiltering(QSGTexture::Nearest);
        dummyTexture->setHorizontalWrapMode(QSGTexture::Repeat);
        dummyTexture->setVerticalWrapMode(QSGTexture::Repeat);
        QImage img(128, 128, QImage::Format_ARGB32_Premultiplied);
        img.fill(0);
        dummyTexture->setImage(img);
        dummyTexture->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());

        m->setDummyTexture(dummyTexture);
    }

    *texture = m->dummyTexture();
}

QQuickShapeTextureFillMaterial::~QQuickShapeTextureFillMaterial()
{
    delete m_dummyTexture;
}

QSGMaterialType *QQuickShapeTextureFillMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

int QQuickShapeTextureFillMaterial::compare(const QSGMaterial *other) const
{
    Q_ASSERT(other && type() == other->type());
    const QQuickShapeTextureFillMaterial *m = static_cast<const QQuickShapeTextureFillMaterial *>(other);

    QQuickShapeGenericStrokeFillNode *a = node();
    QQuickShapeGenericStrokeFillNode *b = m->node();
    Q_ASSERT(a && b);
    if (a == b)
        return 0;

    if (int d = a->m_fillTransform.compareTo(b->m_fillTransform))
        return d;

    const qintptr diff = qintptr(a->m_fillTextureProvider) - qintptr(b->m_fillTextureProvider);
    return diff < 0 ? -1 : (diff > 0 ? 1 : 0);
}

QSGMaterialShader *QQuickShapeTextureFillMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    return new QQuickShapeTextureFillRhiShader(viewCount());
}

QT_END_NAMESPACE

#include "moc_qquickshapegenericrenderer_p.cpp"
