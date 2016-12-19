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

QQuickPathItemPrivate::QQuickPathItemPrivate()
    : rendererType(QQuickPathItem::UnknownRenderer),
      renderer(nullptr),
      path(nullptr),
      dirty(DirtyAll),
      strokeColor(Qt::white),
      strokeWidth(1),
      fillColor(Qt::white),
      fillRule(QQuickPathItem::OddEvenFill),
      joinStyle(QQuickPathItem::BevelJoin),
      miterLimit(2),
      capStyle(QQuickPathItem::SquareCap),
      strokeStyle(QQuickPathItem::SolidLine),
      dashOffset(0),
      fillGradient(nullptr)
{
    dashPattern << 4 << 2; // 4 * strokeWidth dash followed by 2 * strokeWidth space
}

QQuickPathItemPrivate::~QQuickPathItemPrivate()
{
    delete renderer;
}

/*!
    \qmltype PathItem
    \instantiates QQuickPathItem
    \inqmlmodule QtQuick
    \ingroup qtquick-paths
    \ingroup qtquick-views
    \inherits Item
    \brief Renders a path

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

    \note The types for specifying path elements are shared between PathView
    and PathItem. However, not all PathItem implementations support all path
    element types, while some may not make sense for PathView. PathItem's
    currently supported subset is: PathMove, PathLine, PathQuad, PathCubic,
    PathArc.

    \sa Path, PathMove, PathLine, PathQuad, PathCubic, PathArc
*/

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

/*!
    \qmlproperty Path QtQuick::PathItem::path
    This property holds the path to be rendered.
    For more information see the \l Path documentation.
*/
QQuickPath *QQuickPathItem::path() const
{
    Q_D(const QQuickPathItem);
    return d->path;
}

void QQuickPathItem::setPath(QQuickPath *path)
{
    Q_D(QQuickPathItem);
    if (d->path == path)
        return;

    if (d->path)
        qmlobject_disconnect(d->path, QQuickPath, SIGNAL(changed()),
                             this, QQuickPathItem, SLOT(_q_pathChanged()));
    d->path = path;
    qmlobject_connect(d->path, QQuickPath, SIGNAL(changed()),
                      this, QQuickPathItem, SLOT(_q_pathChanged()));

    d->dirty |= QQuickPathItemPrivate::DirtyPath;
    emit pathChanged();
    polish();
}

void QQuickPathItemPrivate::_q_pathChanged()
{
    Q_Q(QQuickPathItem);
    dirty |= DirtyPath;
    q->polish();
}

QColor QQuickPathItem::strokeColor() const
{
    Q_D(const QQuickPathItem);
    return d->strokeColor;
}

void QQuickPathItem::setStrokeColor(const QColor &color)
{
    Q_D(QQuickPathItem);
    if (d->strokeColor != color) {
        d->strokeColor = color;
        d->dirty |= QQuickPathItemPrivate::DirtyStrokeColor;
        emit strokeColorChanged();
        polish();
    }
}

qreal QQuickPathItem::strokeWidth() const
{
    Q_D(const QQuickPathItem);
    return d->strokeWidth;
}

void QQuickPathItem::setStrokeWidth(qreal w)
{
    Q_D(QQuickPathItem);
    if (d->strokeWidth != w) {
        d->strokeWidth = w;
        d->dirty |= QQuickPathItemPrivate::DirtyStrokeWidth;
        emit strokeWidthChanged();
        polish();
    }
}

QColor QQuickPathItem::fillColor() const
{
    Q_D(const QQuickPathItem);
    return d->fillColor;
}

void QQuickPathItem::setFillColor(const QColor &color)
{
    Q_D(QQuickPathItem);
    if (d->fillColor != color) {
        d->fillColor = color;
        d->dirty |= QQuickPathItemPrivate::DirtyFillColor;
        emit fillColorChanged();
        polish();
    }
}

QQuickPathItem::FillRule QQuickPathItem::fillRule() const
{
    Q_D(const QQuickPathItem);
    return d->fillRule;
}

void QQuickPathItem::setFillRule(FillRule fillRule)
{
    Q_D(QQuickPathItem);
    if (d->fillRule != fillRule) {
        d->fillRule = fillRule;
        d->dirty |= QQuickPathItemPrivate::DirtyFillRule;
        emit fillRuleChanged();
        polish();
    }
}

QQuickPathItem::JoinStyle QQuickPathItem::joinStyle() const
{
    Q_D(const QQuickPathItem);
    return d->joinStyle;
}

void QQuickPathItem::setJoinStyle(JoinStyle style)
{
    Q_D(QQuickPathItem);
    if (d->joinStyle != style) {
        d->joinStyle = style;
        d->dirty |= QQuickPathItemPrivate::DirtyStyle;
        emit joinStyleChanged();
        polish();
    }
}

int QQuickPathItem::miterLimit() const
{
    Q_D(const QQuickPathItem);
    return d->miterLimit;
}

void QQuickPathItem::setMiterLimit(int limit)
{
    Q_D(QQuickPathItem);
    if (d->miterLimit != limit) {
        d->miterLimit = limit;
        d->dirty |= QQuickPathItemPrivate::DirtyStyle;
        emit miterLimitChanged();
        polish();
    }
}

QQuickPathItem::CapStyle QQuickPathItem::capStyle() const
{
    Q_D(const QQuickPathItem);
    return d->capStyle;
}

void QQuickPathItem::setCapStyle(CapStyle style)
{
    Q_D(QQuickPathItem);
    if (d->capStyle != style) {
        d->capStyle = style;
        d->dirty |= QQuickPathItemPrivate::DirtyStyle;
        emit capStyleChanged();
        polish();
    }
}

QQuickPathItem::StrokeStyle QQuickPathItem::strokeStyle() const
{
    Q_D(const QQuickPathItem);
    return d->strokeStyle;
}

void QQuickPathItem::setStrokeStyle(StrokeStyle style)
{
    Q_D(QQuickPathItem);
    if (d->strokeStyle != style) {
        d->strokeStyle = style;
        d->dirty |= QQuickPathItemPrivate::DirtyDash;
        emit strokeStyleChanged();
        polish();
    }
}

qreal QQuickPathItem::dashOffset() const
{
    Q_D(const QQuickPathItem);
    return d->dashOffset;
}

void QQuickPathItem::setDashOffset(qreal offset)
{
    Q_D(QQuickPathItem);
    if (d->dashOffset != offset) {
        d->dashOffset = offset;
        d->dirty |= QQuickPathItemPrivate::DirtyDash;
        emit dashOffsetChanged();
        polish();
    }
}

QVector<qreal> QQuickPathItem::dashPattern() const
{
    Q_D(const QQuickPathItem);
    return d->dashPattern;
}

void QQuickPathItem::setDashPattern(const QVector<qreal> &array)
{
    Q_D(QQuickPathItem);
    if (d->dashPattern != array) {
        d->dashPattern = array;
        d->dirty |= QQuickPathItemPrivate::DirtyDash;
        emit dashPatternChanged();
        polish();
    }
}

QQuickPathGradient *QQuickPathItem::fillGradient() const
{
    Q_D(const QQuickPathItem);
    return d->fillGradient;
}

void QQuickPathItem::setFillGradient(QQuickPathGradient *gradient)
{
    Q_D(QQuickPathItem);
    if (d->fillGradient != gradient) {
        if (d->fillGradient)
            qmlobject_disconnect(d->fillGradient, QQuickPathGradient, SIGNAL(updated()),
                                 this, QQuickPathItem, SLOT(_q_fillGradientChanged()));
        d->fillGradient = gradient;
        if (d->fillGradient)
            qmlobject_connect(d->fillGradient, QQuickPathGradient, SIGNAL(updated()),
                                 this, QQuickPathItem, SLOT(_q_fillGradientChanged()));
        d->dirty |= QQuickPathItemPrivate::DirtyFillGradient;
        polish();
    }
}

void QQuickPathItemPrivate::_q_fillGradientChanged()
{
    Q_Q(QQuickPathItem);
    dirty |= DirtyFillGradient;
    q->polish();
}

void QQuickPathItem::resetFillGradient()
{
    setFillGradient(nullptr);
}

void QQuickPathItem::updatePolish()
{
    Q_D(QQuickPathItem);

    if (!d->dirty)
        return;

    if (!d->renderer) {
        d->createRenderer();
        if (!d->renderer)
            return;
        emit rendererChanged();
    }

    // endSync() is where expensive calculations may happen, depending on the
    // backend. Therefore do this only when the item is visible.
    if (isVisible())
        d->sync();

    update();
}

void QQuickPathItem::itemChange(ItemChange change, const ItemChangeData &data)
{
    // sync may have been deferred; do it now if the item became visible
    if (change == ItemVisibleHasChanged && data.boolValue)
        polish();

    QQuickItem::itemChange(change, data);
}

QSGNode *QQuickPathItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    // Called on the render thread, with the gui thread blocked. We can now
    // safely access gui thread data.

    Q_D(QQuickPathItem);
    if (d->renderer) {
        if (!node)
            node = d->createRenderNode();
        d->renderer->updatePathRenderNode();
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
            renderer = new QQuickPathItemNvprRenderer(q);
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
QSGNode *QQuickPathItemPrivate::createRenderNode()
{
    Q_Q(QQuickPathItem);
    QSGNode *node = nullptr;
    if (!q->window())
        return node;
    QSGRendererInterface *ri = q->window()->rendererInterface();
    if (!ri)
        return node;

    const bool hasFill = fillColor != Qt::transparent;
    const bool hasStroke = strokeWidth >= 0.0f && strokeColor != Qt::transparent;

    switch (ri->graphicsApi()) {
#ifndef QT_NO_OPENGL
    case QSGRendererInterface::OpenGL:
        if (QQuickPathItemNvprRenderNode::isSupported()) {
            node = new QQuickPathItemNvprRenderNode(q);
            static_cast<QQuickPathItemNvprRenderer *>(renderer)->setNode(
                static_cast<QQuickPathItemNvprRenderNode *>(node));
        } else {
            node = new QQuickPathItemGenericRootRenderNode(q->window(), hasFill, hasStroke);
            static_cast<QQuickPathItemGenericRenderer *>(renderer)->setRootNode(
                static_cast<QQuickPathItemGenericRootRenderNode *>(node));
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

void QQuickPathItemPrivate::sync()
{
    renderer->beginSync();

    if (dirty & QQuickPathItemPrivate::DirtyPath)
        renderer->setPath(path);
    if (dirty & DirtyStrokeColor)
        renderer->setStrokeColor(strokeColor);
    if (dirty & DirtyStrokeWidth)
        renderer->setStrokeWidth(strokeWidth);
    if (dirty & DirtyFillColor)
        renderer->setFillColor(fillColor);
    if (dirty & DirtyFillRule)
        renderer->setFillRule(fillRule);
    if (dirty & DirtyStyle) {
        renderer->setJoinStyle(joinStyle, miterLimit);
        renderer->setCapStyle(capStyle);
    }
    if (dirty & DirtyDash)
        renderer->setStrokeStyle(strokeStyle, dashOffset, dashPattern);
    if (dirty & DirtyFillGradient)
        renderer->setFillGradient(fillGradient);

    renderer->endSync();
    dirty = 0;
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
