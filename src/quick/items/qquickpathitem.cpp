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

#include "qquickpathitem_p.h"
#include "qquickpathitem_p_p.h"
#include "qquickpathitemgenericrenderer_p.h"
#include "qquickpathitemnvprrenderer_p.h"
#include "qquickpathitemsoftwarerenderer_p.h"
#include <private/qsgtexture_p.h>
#include <QtGui/private/qdrawhelper_p.h>
#include <QOpenGLFunctions>

QT_BEGIN_NAMESPACE

QQuickVisualPathPrivate::QQuickVisualPathPrivate()
    : path(nullptr),
      dirty(DirtyAll),
      strokeColor(Qt::white),
      strokeWidth(1),
      fillColor(Qt::white),
      fillRule(QQuickVisualPath::OddEvenFill),
      joinStyle(QQuickVisualPath::BevelJoin),
      miterLimit(2),
      capStyle(QQuickVisualPath::SquareCap),
      strokeStyle(QQuickVisualPath::SolidLine),
      dashOffset(0),
      fillGradient(nullptr)
{
    dashPattern << 4 << 2; // 4 * strokeWidth dash followed by 2 * strokeWidth space
}

QQuickVisualPath::QQuickVisualPath(QObject *parent)
    : QObject(*(new QQuickVisualPathPrivate), parent)
{
}

QQuickVisualPath::~QQuickVisualPath()
{
}

QQuickPath *QQuickVisualPath::path() const
{
    Q_D(const QQuickVisualPath);
    return d->path;
}

void QQuickVisualPath::setPath(QQuickPath *path)
{
    Q_D(QQuickVisualPath);
    if (d->path == path)
        return;

    if (d->path)
        qmlobject_disconnect(d->path, QQuickPath, SIGNAL(changed()),
                             this, QQuickVisualPath, SLOT(_q_pathChanged()));
    d->path = path;
    qmlobject_connect(d->path, QQuickPath, SIGNAL(changed()),
                      this, QQuickVisualPath, SLOT(_q_pathChanged()));

    d->dirty |= QQuickVisualPathPrivate::DirtyPath;
    emit pathChanged();
    emit changed();
}

void QQuickVisualPathPrivate::_q_pathChanged()
{
    Q_Q(QQuickVisualPath);
    dirty |= DirtyPath;
    emit q->changed();
}

QColor QQuickVisualPath::strokeColor() const
{
    Q_D(const QQuickVisualPath);
    return d->strokeColor;
}

void QQuickVisualPath::setStrokeColor(const QColor &color)
{
    Q_D(QQuickVisualPath);
    if (d->strokeColor != color) {
        d->strokeColor = color;
        d->dirty |= QQuickVisualPathPrivate::DirtyStrokeColor;
        emit strokeColorChanged();
        emit changed();
    }
}

qreal QQuickVisualPath::strokeWidth() const
{
    Q_D(const QQuickVisualPath);
    return d->strokeWidth;
}

void QQuickVisualPath::setStrokeWidth(qreal w)
{
    Q_D(QQuickVisualPath);
    if (d->strokeWidth != w) {
        d->strokeWidth = w;
        d->dirty |= QQuickVisualPathPrivate::DirtyStrokeWidth;
        emit strokeWidthChanged();
        emit changed();
    }
}

QColor QQuickVisualPath::fillColor() const
{
    Q_D(const QQuickVisualPath);
    return d->fillColor;
}

void QQuickVisualPath::setFillColor(const QColor &color)
{
    Q_D(QQuickVisualPath);
    if (d->fillColor != color) {
        d->fillColor = color;
        d->dirty |= QQuickVisualPathPrivate::DirtyFillColor;
        emit fillColorChanged();
        emit changed();
    }
}

QQuickVisualPath::FillRule QQuickVisualPath::fillRule() const
{
    Q_D(const QQuickVisualPath);
    return d->fillRule;
}

void QQuickVisualPath::setFillRule(FillRule fillRule)
{
    Q_D(QQuickVisualPath);
    if (d->fillRule != fillRule) {
        d->fillRule = fillRule;
        d->dirty |= QQuickVisualPathPrivate::DirtyFillRule;
        emit fillRuleChanged();
        emit changed();
    }
}

QQuickVisualPath::JoinStyle QQuickVisualPath::joinStyle() const
{
    Q_D(const QQuickVisualPath);
    return d->joinStyle;
}

void QQuickVisualPath::setJoinStyle(JoinStyle style)
{
    Q_D(QQuickVisualPath);
    if (d->joinStyle != style) {
        d->joinStyle = style;
        d->dirty |= QQuickVisualPathPrivate::DirtyStyle;
        emit joinStyleChanged();
        emit changed();
    }
}

int QQuickVisualPath::miterLimit() const
{
    Q_D(const QQuickVisualPath);
    return d->miterLimit;
}

void QQuickVisualPath::setMiterLimit(int limit)
{
    Q_D(QQuickVisualPath);
    if (d->miterLimit != limit) {
        d->miterLimit = limit;
        d->dirty |= QQuickVisualPathPrivate::DirtyStyle;
        emit miterLimitChanged();
        emit changed();
    }
}

QQuickVisualPath::CapStyle QQuickVisualPath::capStyle() const
{
    Q_D(const QQuickVisualPath);
    return d->capStyle;
}

void QQuickVisualPath::setCapStyle(CapStyle style)
{
    Q_D(QQuickVisualPath);
    if (d->capStyle != style) {
        d->capStyle = style;
        d->dirty |= QQuickVisualPathPrivate::DirtyStyle;
        emit capStyleChanged();
        emit changed();
    }
}

QQuickVisualPath::StrokeStyle QQuickVisualPath::strokeStyle() const
{
    Q_D(const QQuickVisualPath);
    return d->strokeStyle;
}

void QQuickVisualPath::setStrokeStyle(StrokeStyle style)
{
    Q_D(QQuickVisualPath);
    if (d->strokeStyle != style) {
        d->strokeStyle = style;
        d->dirty |= QQuickVisualPathPrivate::DirtyDash;
        emit strokeStyleChanged();
        emit changed();
    }
}

qreal QQuickVisualPath::dashOffset() const
{
    Q_D(const QQuickVisualPath);
    return d->dashOffset;
}

void QQuickVisualPath::setDashOffset(qreal offset)
{
    Q_D(QQuickVisualPath);
    if (d->dashOffset != offset) {
        d->dashOffset = offset;
        d->dirty |= QQuickVisualPathPrivate::DirtyDash;
        emit dashOffsetChanged();
        emit changed();
    }
}

QVector<qreal> QQuickVisualPath::dashPattern() const
{
    Q_D(const QQuickVisualPath);
    return d->dashPattern;
}

void QQuickVisualPath::setDashPattern(const QVector<qreal> &array)
{
    Q_D(QQuickVisualPath);
    if (d->dashPattern != array) {
        d->dashPattern = array;
        d->dirty |= QQuickVisualPathPrivate::DirtyDash;
        emit dashPatternChanged();
        emit changed();
    }
}

QQuickPathGradient *QQuickVisualPath::fillGradient() const
{
    Q_D(const QQuickVisualPath);
    return d->fillGradient;
}

void QQuickVisualPath::setFillGradient(QQuickPathGradient *gradient)
{
    Q_D(QQuickVisualPath);
    if (d->fillGradient != gradient) {
        if (d->fillGradient)
            qmlobject_disconnect(d->fillGradient, QQuickPathGradient, SIGNAL(updated()),
                                 this, QQuickVisualPath, SLOT(_q_fillGradientChanged()));
        d->fillGradient = gradient;
        if (d->fillGradient)
            qmlobject_connect(d->fillGradient, QQuickPathGradient, SIGNAL(updated()),
                              this, QQuickVisualPath, SLOT(_q_fillGradientChanged()));
        d->dirty |= QQuickVisualPathPrivate::DirtyFillGradient;
        emit changed();
    }
}

void QQuickVisualPathPrivate::_q_fillGradientChanged()
{
    Q_Q(QQuickVisualPath);
    dirty |= DirtyFillGradient;
    emit q->changed();
}

void QQuickVisualPath::resetFillGradient()
{
    setFillGradient(nullptr);
}

/*!
    \qmltype PathItem
    \instantiates QQuickPathItem
    \inqmlmodule QtQuick
    \ingroup qtquick-paths
    \ingroup qtquick-views
    \inherits Item
    \brief Renders a path
    \since 5.10

    Renders a path either by generating geometry via QPainterPath and manual
    triangulation or by using an extension like \c{GL_NV_path_rendering}.

    This approach is different from rendering shapes via QQuickPaintedItem or
    the 2D Canvas because the path never gets rasterized in software. Therefore
    it is suitable for creating shapes spreading over larger areas of the
    screen, avoiding the performance penalty for texture uploads or framebuffer
    blits.

    Nonetheless it is important to be aware of performance implications, in
    particular when the application is running on the generic PathItem
    implementation due to not having support for accelerated path rendering.
    The geometry generation happens entirely on the CPU in this case, and this
    is potentially expensive. Changing the set of path elements, changing the
    properties of these elements, or changing certain properties of the
    PathItem itself all lead to retriangulation on every change. Therefore,
    applying animation to such properties can heavily affect performance on
    less powerful systems. If animating properties other than stroke and fill
    colors is a must, it is recommended to target systems providing
    \c{GL_NV_path_rendering} where the cost of path property changes is much
    smaller.

    \note The types for specifying path elements are shared between \l PathView
    and PathItem. However, not all PathItem implementations support all path
    element types, while some may not make sense for PathView. PathItem's
    currently supported subset is: PathMove, PathLine, PathQuad, PathCubic,
    PathArc.

    \note Limited support for PathSvg is also provided in most cases. However,
    there is no guarantee that this element is going to be supported for all
    future PathItem backends. It is recommended to avoid the PathSvg element in
    practice.

    See \l Path for a detailed overview of the supported path elements.

    \sa Path, PathMove, PathLine, PathQuad, PathCubic, PathArc, PathSvg
*/

QQuickPathItemPrivate::QQuickPathItemPrivate()
    : componentComplete(true),
      vpChanged(false),
      rendererType(QQuickPathItem::UnknownRenderer),
      async(false),
      status(QQuickPathItem::Null),
      renderer(nullptr)
{
}

QQuickPathItemPrivate::~QQuickPathItemPrivate()
{
    delete renderer;
}

void QQuickPathItemPrivate::_q_visualPathChanged()
{
    Q_Q(QQuickPathItem);
    vpChanged = true;
    q->polish();
}

void QQuickPathItemPrivate::setStatus(QQuickPathItem::Status newStatus)
{
    Q_Q(QQuickPathItem);
    if (status != newStatus) {
        status = newStatus;
        emit q->statusChanged();
    }
}

QQuickPathItem::QQuickPathItem(QQuickItem *parent)
  : QQuickItem(*(new QQuickPathItemPrivate), parent)
{
    setFlag(ItemHasContents);
}

QQuickPathItem::~QQuickPathItem()
{
}

QQuickPathItem::RendererType QQuickPathItem::rendererType() const
{
    Q_D(const QQuickPathItem);
    return d->rendererType;
}

bool QQuickPathItem::asynchronous() const
{
    Q_D(const QQuickPathItem);
    return d->async;
}

void QQuickPathItem::setAsynchronous(bool async)
{
    Q_D(QQuickPathItem);
    if (d->async != async) {
        d->async = async;
        emit asynchronousChanged();
        if (d->componentComplete)
            d->_q_visualPathChanged();
    }
}

QQuickPathItem::Status QQuickPathItem::status() const
{
    Q_D(const QQuickPathItem);
    return d->status;
}

static QQuickVisualPath *vpe_at(QQmlListProperty<QQuickVisualPath> *property, int index)
{
    QQuickPathItemPrivate *d = QQuickPathItemPrivate::get(static_cast<QQuickPathItem *>(property->object));
    return d->vp.at(index);
}

static void vpe_append(QQmlListProperty<QQuickVisualPath> *property, QQuickVisualPath *obj)
{
    QQuickPathItem *item = static_cast<QQuickPathItem *>(property->object);
    QQuickPathItemPrivate *d = QQuickPathItemPrivate::get(item);
    d->vp.append(obj);

    if (d->componentComplete) {
        QObject::connect(obj, SIGNAL(changed()), item, SLOT(_q_visualPathChanged()));
        d->_q_visualPathChanged();
    }
}

static int vpe_count(QQmlListProperty<QQuickVisualPath> *property)
{
    QQuickPathItemPrivate *d = QQuickPathItemPrivate::get(static_cast<QQuickPathItem *>(property->object));
    return d->vp.count();
}

static void vpe_clear(QQmlListProperty<QQuickVisualPath> *property)
{
    QQuickPathItem *item = static_cast<QQuickPathItem *>(property->object);
    QQuickPathItemPrivate *d = QQuickPathItemPrivate::get(item);

    for (QQuickVisualPath *p : d->vp)
        QObject::disconnect(p, SIGNAL(changed()), item, SLOT(_q_visualPathChanged()));

    d->vp.clear();

    if (d->componentComplete)
        d->_q_visualPathChanged();
}

QQmlListProperty<QQuickVisualPath> QQuickPathItem::visualPaths()
{
    return QQmlListProperty<QQuickVisualPath>(this,
                                              nullptr,
                                              vpe_append,
                                              vpe_count,
                                              vpe_at,
                                              vpe_clear);
}

void QQuickPathItem::classBegin()
{
    Q_D(QQuickPathItem);
    d->componentComplete = false;
}

void QQuickPathItem::componentComplete()
{
    Q_D(QQuickPathItem);
    d->componentComplete = true;

    for (QQuickVisualPath *p : d->vp)
        connect(p, SIGNAL(changed()), this, SLOT(_q_visualPathChanged()));

    d->_q_visualPathChanged();
}

void QQuickPathItem::updatePolish()
{
    Q_D(QQuickPathItem);

    if (!d->vpChanged)
        return;

    d->vpChanged = false;

    if (!d->renderer) {
        d->createRenderer();
        if (!d->renderer)
            return;
        emit rendererChanged();
    }

    // endSync() is where expensive calculations may happen (or get kicked off
    // on worker threads), depending on the backend. Therefore do this only
    // when the item is visible.
    if (isVisible())
        d->sync();

    update();
}

void QQuickPathItem::itemChange(ItemChange change, const ItemChangeData &data)
{
    Q_D(QQuickPathItem);

    // sync may have been deferred; do it now if the item became visible
    if (change == ItemVisibleHasChanged && data.boolValue)
        d->_q_visualPathChanged();

    QQuickItem::itemChange(change, data);
}

QSGNode *QQuickPathItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    // Called on the render thread, with the gui thread blocked. We can now
    // safely access gui thread data.

    Q_D(QQuickPathItem);
    if (d->renderer) {
        if (!node)
            node = d->createNode();
        d->renderer->updateNode();
    }
    return node;
}

// the renderer object lives on the gui thread
void QQuickPathItemPrivate::createRenderer()
{
    Q_Q(QQuickPathItem);
    QSGRendererInterface *ri = q->window()->rendererInterface();
    if (!ri)
        return;

    switch (ri->graphicsApi()) {
#ifndef QT_NO_OPENGL
    case QSGRendererInterface::OpenGL:
        if (QQuickPathItemNvprRenderNode::isSupported()) {
            rendererType = QQuickPathItem::NvprRenderer;
            renderer = new QQuickPathItemNvprRenderer;
        } else {
            rendererType = QQuickPathItem::GeometryRenderer;
            renderer = new QQuickPathItemGenericRenderer(q);
        }
        break;
#endif
    case QSGRendererInterface::Software:
        rendererType = QQuickPathItem::SoftwareRenderer;
        renderer = new QQuickPathItemSoftwareRenderer;
        break;
    default:
        qWarning("No path backend for this graphics API yet");
        break;
    }
}

// the node lives on the render thread
QSGNode *QQuickPathItemPrivate::createNode()
{
    Q_Q(QQuickPathItem);
    QSGNode *node = nullptr;
    if (!q->window())
        return node;
    QSGRendererInterface *ri = q->window()->rendererInterface();
    if (!ri)
        return node;

    switch (ri->graphicsApi()) {
#ifndef QT_NO_OPENGL
    case QSGRendererInterface::OpenGL:
        if (QQuickPathItemNvprRenderNode::isSupported()) {
            node = new QQuickPathItemNvprRenderNode(q);
            static_cast<QQuickPathItemNvprRenderer *>(renderer)->setNode(
                static_cast<QQuickPathItemNvprRenderNode *>(node));
        } else {
            node = new QQuickPathItemGenericNode;
            static_cast<QQuickPathItemGenericRenderer *>(renderer)->setRootNode(
                static_cast<QQuickPathItemGenericNode *>(node));
        }
        break;
#endif
    case QSGRendererInterface::Software:
        node = new QQuickPathItemSoftwareRenderNode(q);
        static_cast<QQuickPathItemSoftwareRenderer *>(renderer)->setNode(
                    static_cast<QQuickPathItemSoftwareRenderNode *>(node));
        break;
    default:
        qWarning("No path backend for this graphics API yet");
        break;
    }

    return node;
}

static void q_asyncPathItemReady(void *data)
{
    QQuickPathItemPrivate *self = static_cast<QQuickPathItemPrivate *>(data);
    self->setStatus(QQuickPathItem::Ready);
}

void QQuickPathItemPrivate::sync()
{
    const bool useAsync = async && renderer->flags().testFlag(QQuickAbstractPathRenderer::SupportsAsync);
    if (useAsync) {
        setStatus(QQuickPathItem::Processing);
        renderer->setAsyncCallback(q_asyncPathItemReady, this);
    }

    const int count = vp.count();
    renderer->beginSync(count);

    for (int i = 0; i < count; ++i) {
        QQuickVisualPath *p = vp[i];
        int &dirty(QQuickVisualPathPrivate::get(p)->dirty);

        if (dirty & QQuickVisualPathPrivate::DirtyPath)
            renderer->setPath(i, p->path());
        if (dirty & QQuickVisualPathPrivate::DirtyStrokeColor)
            renderer->setStrokeColor(i, p->strokeColor());
        if (dirty & QQuickVisualPathPrivate::DirtyStrokeWidth)
            renderer->setStrokeWidth(i, p->strokeWidth());
        if (dirty & QQuickVisualPathPrivate::DirtyFillColor)
            renderer->setFillColor(i, p->fillColor());
        if (dirty & QQuickVisualPathPrivate::DirtyFillRule)
            renderer->setFillRule(i, p->fillRule());
        if (dirty & QQuickVisualPathPrivate::DirtyStyle) {
            renderer->setJoinStyle(i, p->joinStyle(), p->miterLimit());
            renderer->setCapStyle(i, p->capStyle());
        }
        if (dirty & QQuickVisualPathPrivate::DirtyDash)
            renderer->setStrokeStyle(i, p->strokeStyle(), p->dashOffset(), p->dashPattern());
        if (dirty & QQuickVisualPathPrivate::DirtyFillGradient)
            renderer->setFillGradient(i, p->fillGradient());

        dirty = 0;
    }

    renderer->endSync(useAsync);

    if (!useAsync)
        setStatus(QQuickPathItem::Ready);
}

// ***** gradient support *****

QQuickPathGradientStop::QQuickPathGradientStop(QObject *parent)
    : QObject(parent),
      m_position(0),
      m_color(Qt::black)
{
}

qreal QQuickPathGradientStop::position() const
{
    return m_position;
}

void QQuickPathGradientStop::setPosition(qreal position)
{
    if (m_position != position) {
        m_position = position;
        if (QQuickPathGradient *grad = qobject_cast<QQuickPathGradient *>(parent()))
            emit grad->updated();
    }
}

QColor QQuickPathGradientStop::color() const
{
    return m_color;
}

void QQuickPathGradientStop::setColor(const QColor &color)
{
    if (m_color != color) {
        m_color = color;
        if (QQuickPathGradient *grad = qobject_cast<QQuickPathGradient *>(parent()))
            emit grad->updated();
    }
}

QQuickPathGradient::QQuickPathGradient(QObject *parent)
    : QObject(parent),
      m_spread(PadSpread)
{
}

void QQuickPathGradient::appendStop(QQmlListProperty<QObject> *list, QObject *stop)
{
    QQuickPathGradientStop *sstop = qobject_cast<QQuickPathGradientStop *>(stop);
    if (!sstop) {
        qWarning("Gradient stop list only supports QQuickPathGradientStop elements");
        return;
    }
    QQuickPathGradient *grad = qobject_cast<QQuickPathGradient *>(list->object);
    Q_ASSERT(grad);
    sstop->setParent(grad);
    grad->m_stops.append(sstop);
}

QQmlListProperty<QObject> QQuickPathGradient::stops()
{
    return QQmlListProperty<QObject>(this, nullptr, &QQuickPathGradient::appendStop, nullptr, nullptr, nullptr);
}

QGradientStops QQuickPathGradient::sortedGradientStops() const
{
    QGradientStops result;
    for (int i = 0; i < m_stops.count(); ++i) {
        QQuickPathGradientStop *s = static_cast<QQuickPathGradientStop *>(m_stops[i]);
        int j = 0;
        while (j < result.count() && result[j].first < s->position())
            ++j;
        result.insert(j, QGradientStop(s->position(), s->color()));
    }
    return result;
}

QQuickPathGradient::SpreadMode QQuickPathGradient::spread() const
{
    return m_spread;
}

void QQuickPathGradient::setSpread(SpreadMode mode)
{
    if (m_spread != mode) {
        m_spread = mode;
        emit spreadChanged();
        emit updated();
    }
}

QQuickPathLinearGradient::QQuickPathLinearGradient(QObject *parent)
    : QQuickPathGradient(parent)
{
}

qreal QQuickPathLinearGradient::x1() const
{
    return m_start.x();
}

void QQuickPathLinearGradient::setX1(qreal v)
{
    if (m_start.x() != v) {
        m_start.setX(v);
        emit x1Changed();
        emit updated();
    }
}

qreal QQuickPathLinearGradient::y1() const
{
    return m_start.y();
}

void QQuickPathLinearGradient::setY1(qreal v)
{
    if (m_start.y() != v) {
        m_start.setY(v);
        emit y1Changed();
        emit updated();
    }
}

qreal QQuickPathLinearGradient::x2() const
{
    return m_end.x();
}

void QQuickPathLinearGradient::setX2(qreal v)
{
    if (m_end.x() != v) {
        m_end.setX(v);
        emit x2Changed();
        emit updated();
    }
}

qreal QQuickPathLinearGradient::y2() const
{
    return m_end.y();
}

void QQuickPathLinearGradient::setY2(qreal v)
{
    if (m_end.y() != v) {
        m_end.setY(v);
        emit y2Changed();
        emit updated();
    }
}

#ifndef QT_NO_OPENGL

// contexts sharing with each other get the same cache instance
class QQuickPathItemGradientCacheWrapper
{
public:
    QQuickPathItemGradientCache *get(QOpenGLContext *context)
    {
        return m_resource.value<QQuickPathItemGradientCache>(context);
    }

private:
    QOpenGLMultiGroupSharedResource m_resource;
};

QQuickPathItemGradientCache *QQuickPathItemGradientCache::currentCache()
{
    static QQuickPathItemGradientCacheWrapper qt_path_gradient_caches;
    return qt_path_gradient_caches.get(QOpenGLContext::currentContext());
}

// let QOpenGLContext manage the lifetime of the cached textures
QQuickPathItemGradientCache::~QQuickPathItemGradientCache()
{
    m_cache.clear();
}

void QQuickPathItemGradientCache::invalidateResource()
{
    m_cache.clear();
}

void QQuickPathItemGradientCache::freeResource(QOpenGLContext *)
{
    qDeleteAll(m_cache);
    m_cache.clear();
}

static void generateGradientColorTable(const QQuickPathItemGradientCache::GradientDesc &gradient,
                                       uint *colorTable, int size, float opacity)
{
    int pos = 0;
    const QGradientStops &s = gradient.stops;
    const bool colorInterpolation = true;

    uint alpha = qRound(opacity * 256);
    uint current_color = ARGB_COMBINE_ALPHA(s[0].second.rgba(), alpha);
    qreal incr = 1.0 / qreal(size);
    qreal fpos = 1.5 * incr;
    colorTable[pos++] = ARGB2RGBA(qPremultiply(current_color));

    while (fpos <= s.first().first) {
        colorTable[pos] = colorTable[pos - 1];
        pos++;
        fpos += incr;
    }

    if (colorInterpolation)
        current_color = qPremultiply(current_color);

    const int sLast = s.size() - 1;
    for (int i = 0; i < sLast; ++i) {
        qreal delta = 1/(s[i+1].first - s[i].first);
        uint next_color = ARGB_COMBINE_ALPHA(s[i + 1].second.rgba(), alpha);
        if (colorInterpolation)
            next_color = qPremultiply(next_color);

        while (fpos < s[i+1].first && pos < size) {
            int dist = int(256 * ((fpos - s[i].first) * delta));
            int idist = 256 - dist;
            if (colorInterpolation)
                colorTable[pos] = ARGB2RGBA(INTERPOLATE_PIXEL_256(current_color, idist, next_color, dist));
            else
                colorTable[pos] = ARGB2RGBA(qPremultiply(INTERPOLATE_PIXEL_256(current_color, idist, next_color, dist)));
            ++pos;
            fpos += incr;
        }
        current_color = next_color;
    }

    Q_ASSERT(s.size() > 0);

    uint last_color = ARGB2RGBA(qPremultiply(ARGB_COMBINE_ALPHA(s[sLast].second.rgba(), alpha)));
    for ( ; pos < size; ++pos)
        colorTable[pos] = last_color;

    colorTable[size-1] = last_color;
}

QSGTexture *QQuickPathItemGradientCache::get(const GradientDesc &grad)
{
    QSGPlainTexture *tx = m_cache[grad];
    if (!tx) {
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        GLuint id;
        f->glGenTextures(1, &id);
        f->glBindTexture(GL_TEXTURE_2D, id);
        static const uint W = 1024; // texture size is 1024x1
        uint buf[W];
        generateGradientColorTable(grad, buf, W, 1.0f);
        f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
        tx = new QSGPlainTexture;
        tx->setTextureId(id);
        switch (grad.spread) {
        case QQuickPathGradient::PadSpread:
            tx->setHorizontalWrapMode(QSGTexture::ClampToEdge);
            tx->setVerticalWrapMode(QSGTexture::ClampToEdge);
            break;
        case QQuickPathGradient::RepeatSpread:
            tx->setHorizontalWrapMode(QSGTexture::Repeat);
            tx->setVerticalWrapMode(QSGTexture::Repeat);
            break;
        case QQuickPathGradient::ReflectSpread:
            tx->setHorizontalWrapMode(QSGTexture::MirroredRepeat);
            tx->setVerticalWrapMode(QSGTexture::MirroredRepeat);
            break;
        default:
            qWarning("Unknown gradient spread mode %d", grad.spread);
            break;
        }
        m_cache[grad] = tx;
    }
    return tx;
}

#endif // QT_NO_OPENGL

QT_END_NAMESPACE

#include "moc_qquickpathitem_p.cpp"
