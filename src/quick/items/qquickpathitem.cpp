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
#include <private/qquicksvgparser_p.h>
#include <QtGui/private/qdrawhelper_p.h>
#include <QOpenGLFunctions>

#include <private/qv4engine_p.h>
#include <private/qv4object_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4mm_p.h>
#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

QQuickPathItemStrokeFillParams::QQuickPathItemStrokeFillParams()
    : strokeColor(Qt::white),
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

QPainterPath QQuickPathItemPath::toPainterPath() const
{
    QPainterPath p;
    int coordIdx = 0;
    for (int i = 0; i < cmd.count(); ++i) {
        switch (cmd[i]) {
        case QQuickPathItemPath::MoveTo:
            p.moveTo(coords[coordIdx], coords[coordIdx + 1]);
            coordIdx += 2;
            break;
        case QQuickPathItemPath::LineTo:
            p.lineTo(coords[coordIdx], coords[coordIdx + 1]);
            coordIdx += 2;
            break;
        case QQuickPathItemPath::QuadTo:
            p.quadTo(coords[coordIdx], coords[coordIdx + 1],
                    coords[coordIdx + 2], coords[coordIdx + 3]);
            coordIdx += 4;
            break;
        case QQuickPathItemPath::CubicTo:
            p.cubicTo(coords[coordIdx], coords[coordIdx + 1],
                    coords[coordIdx + 2], coords[coordIdx + 3],
                    coords[coordIdx + 4], coords[coordIdx + 5]);
            coordIdx += 6;
            break;
        case QQuickPathItemPath::ArcTo:
            // does not map to the QPainterPath API; reuse the helper code from QQuickSvgParser
            QQuickSvgParser::pathArc(p,
                                     coords[coordIdx], coords[coordIdx + 1], // radius
                                     coords[coordIdx + 2], // xAxisRotation
                                     !qFuzzyIsNull(coords[coordIdx + 6]), // useLargeArc
                                     !qFuzzyIsNull(coords[coordIdx + 5]), // sweep flag
                                     coords[coordIdx + 3], coords[coordIdx + 4], // end
                                     p.currentPosition().x(), p.currentPosition().y());
            coordIdx += 7;
            break;
        default:
            qWarning("Unknown JS path command: %d", cmd[i]);
            break;
        }
    }
    return p;
}

QQuickVisualPathPrivate::QQuickVisualPathPrivate()
    : path(nullptr),
      dirty(DirtyAll)
{
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
    return d->sfp.strokeColor;
}

void QQuickVisualPath::setStrokeColor(const QColor &color)
{
    Q_D(QQuickVisualPath);
    if (d->sfp.strokeColor != color) {
        d->sfp.strokeColor = color;
        d->dirty |= QQuickVisualPathPrivate::DirtyStrokeColor;
        emit strokeColorChanged();
        emit changed();
    }
}

qreal QQuickVisualPath::strokeWidth() const
{
    Q_D(const QQuickVisualPath);
    return d->sfp.strokeWidth;
}

void QQuickVisualPath::setStrokeWidth(qreal w)
{
    Q_D(QQuickVisualPath);
    if (d->sfp.strokeWidth != w) {
        d->sfp.strokeWidth = w;
        d->dirty |= QQuickVisualPathPrivate::DirtyStrokeWidth;
        emit strokeWidthChanged();
        emit changed();
    }
}

QColor QQuickVisualPath::fillColor() const
{
    Q_D(const QQuickVisualPath);
    return d->sfp.fillColor;
}

void QQuickVisualPath::setFillColor(const QColor &color)
{
    Q_D(QQuickVisualPath);
    if (d->sfp.fillColor != color) {
        d->sfp.fillColor = color;
        d->dirty |= QQuickVisualPathPrivate::DirtyFillColor;
        emit fillColorChanged();
        emit changed();
    }
}

QQuickVisualPath::FillRule QQuickVisualPath::fillRule() const
{
    Q_D(const QQuickVisualPath);
    return d->sfp.fillRule;
}

void QQuickVisualPath::setFillRule(FillRule fillRule)
{
    Q_D(QQuickVisualPath);
    if (d->sfp.fillRule != fillRule) {
        d->sfp.fillRule = fillRule;
        d->dirty |= QQuickVisualPathPrivate::DirtyFillRule;
        emit fillRuleChanged();
        emit changed();
    }
}

QQuickVisualPath::JoinStyle QQuickVisualPath::joinStyle() const
{
    Q_D(const QQuickVisualPath);
    return d->sfp.joinStyle;
}

void QQuickVisualPath::setJoinStyle(JoinStyle style)
{
    Q_D(QQuickVisualPath);
    if (d->sfp.joinStyle != style) {
        d->sfp.joinStyle = style;
        d->dirty |= QQuickVisualPathPrivate::DirtyStyle;
        emit joinStyleChanged();
        emit changed();
    }
}

int QQuickVisualPath::miterLimit() const
{
    Q_D(const QQuickVisualPath);
    return d->sfp.miterLimit;
}

void QQuickVisualPath::setMiterLimit(int limit)
{
    Q_D(QQuickVisualPath);
    if (d->sfp.miterLimit != limit) {
        d->sfp.miterLimit = limit;
        d->dirty |= QQuickVisualPathPrivate::DirtyStyle;
        emit miterLimitChanged();
        emit changed();
    }
}

QQuickVisualPath::CapStyle QQuickVisualPath::capStyle() const
{
    Q_D(const QQuickVisualPath);
    return d->sfp.capStyle;
}

void QQuickVisualPath::setCapStyle(CapStyle style)
{
    Q_D(QQuickVisualPath);
    if (d->sfp.capStyle != style) {
        d->sfp.capStyle = style;
        d->dirty |= QQuickVisualPathPrivate::DirtyStyle;
        emit capStyleChanged();
        emit changed();
    }
}

QQuickVisualPath::StrokeStyle QQuickVisualPath::strokeStyle() const
{
    Q_D(const QQuickVisualPath);
    return d->sfp.strokeStyle;
}

void QQuickVisualPath::setStrokeStyle(StrokeStyle style)
{
    Q_D(QQuickVisualPath);
    if (d->sfp.strokeStyle != style) {
        d->sfp.strokeStyle = style;
        d->dirty |= QQuickVisualPathPrivate::DirtyDash;
        emit strokeStyleChanged();
        emit changed();
    }
}

qreal QQuickVisualPath::dashOffset() const
{
    Q_D(const QQuickVisualPath);
    return d->sfp.dashOffset;
}

void QQuickVisualPath::setDashOffset(qreal offset)
{
    Q_D(QQuickVisualPath);
    if (d->sfp.dashOffset != offset) {
        d->sfp.dashOffset = offset;
        d->dirty |= QQuickVisualPathPrivate::DirtyDash;
        emit dashOffsetChanged();
        emit changed();
    }
}

QVector<qreal> QQuickVisualPath::dashPattern() const
{
    Q_D(const QQuickVisualPath);
    return d->sfp.dashPattern;
}

void QQuickVisualPath::setDashPattern(const QVector<qreal> &array)
{
    Q_D(QQuickVisualPath);
    if (d->sfp.dashPattern != array) {
        d->sfp.dashPattern = array;
        d->dirty |= QQuickVisualPathPrivate::DirtyDash;
        emit dashPatternChanged();
        emit changed();
    }
}

QQuickPathGradient *QQuickVisualPath::fillGradient() const
{
    Q_D(const QQuickVisualPath);
    return d->sfp.fillGradient;
}

void QQuickVisualPath::setFillGradient(QQuickPathGradient *gradient)
{
    Q_D(QQuickVisualPath);
    if (d->sfp.fillGradient != gradient) {
        if (d->sfp.fillGradient)
            qmlobject_disconnect(d->sfp.fillGradient, QQuickPathGradient, SIGNAL(updated()),
                                 this, QQuickVisualPath, SLOT(_q_fillGradientChanged()));
        d->sfp.fillGradient = gradient;
        if (d->sfp.fillGradient)
            qmlobject_connect(d->sfp.fillGradient, QQuickPathGradient, SIGNAL(updated()),
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
    return d->qmlData.vp.at(index);
}

static void vpe_append(QQmlListProperty<QQuickVisualPath> *property, QQuickVisualPath *obj)
{
    QQuickPathItem *item = static_cast<QQuickPathItem *>(property->object);
    QQuickPathItemPrivate *d = QQuickPathItemPrivate::get(item);
    d->qmlData.vp.append(obj);

    if (d->componentComplete) {
        QObject::connect(obj, SIGNAL(changed()), item, SLOT(_q_visualPathChanged()));
        d->_q_visualPathChanged();
    }
}

static int vpe_count(QQmlListProperty<QQuickVisualPath> *property)
{
    QQuickPathItemPrivate *d = QQuickPathItemPrivate::get(static_cast<QQuickPathItem *>(property->object));
    return d->qmlData.vp.count();
}

static void vpe_clear(QQmlListProperty<QQuickVisualPath> *property)
{
    QQuickPathItem *item = static_cast<QQuickPathItem *>(property->object);
    QQuickPathItemPrivate *d = QQuickPathItemPrivate::get(item);

    for (QQuickVisualPath *p : d->qmlData.vp)
        QObject::disconnect(p, SIGNAL(changed()), item, SLOT(_q_visualPathChanged()));

    d->qmlData.vp.clear();

    if (d->componentComplete)
        d->_q_visualPathChanged();
}

QQmlListProperty<QQuickVisualPath> QQuickPathItem::elements()
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

    for (QQuickVisualPath *p : d->qmlData.vp)
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
            node = new QQuickPathItemNvprRenderNode;
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

    if (!jsData.isValid()) {
        // Standard route: The path and stroke/fill parameters are provided via
        // VisualPath and Path.
        const int count = qmlData.vp.count();
        renderer->beginSync(count);

        for (int i = 0; i < count; ++i) {
            QQuickVisualPath *p = qmlData.vp[i];
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
    } else {
        // Path and stroke/fill params provided from JavaScript. This avoids
        // QObjects at the expense of not supporting changes afterwards.
        const int count = jsData.paths.count();
        renderer->beginSync(count);

        for (int i = 0; i < count; ++i) {
            renderer->setJSPath(i, jsData.paths[i]);
            const QQuickPathItemStrokeFillParams sfp(jsData.sfp[i]);
            renderer->setStrokeColor(i, sfp.strokeColor);
            renderer->setStrokeWidth(i, sfp.strokeWidth);
            renderer->setFillColor(i, sfp.fillColor);
            renderer->setFillRule(i, sfp.fillRule);
            renderer->setJoinStyle(i, sfp.joinStyle, sfp.miterLimit);
            renderer->setCapStyle(i, sfp.capStyle);
            renderer->setStrokeStyle(i, sfp.strokeStyle, sfp.dashOffset, sfp.dashPattern);
            renderer->setFillGradient(i, sfp.fillGradient);
        }

        renderer->endSync(useAsync);
    }

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

// ***** JS-based alternative for creating static paths, (mostly) without QObjects *****

class QQuickPathItemJSEngineData : public QV8Engine::Deletable
{
public:
    QQuickPathItemJSEngineData(QV4::ExecutionEngine *engine);

    QV4::PersistentValue pathProto;
    QV4::PersistentValue strokeFillParamsProto;
};

V4_DEFINE_EXTENSION(QQuickPathItemJSEngineData, engineData)

namespace QV4 {
namespace Heap {

struct QQuickPathItemJSPathPrototype : Object {
    void init() { Object::init(); }
};

struct QQuickPathItemJSPath : Object {
    void init() { Object::init(); }
    QQuickPathItemPathObject *obj;
};

struct QQuickPathItemJSStrokeFillParamsPrototype : Object {
    void init() { Object::init(); }
};

struct QQuickPathItemJSStrokeFillParams : Object {
    void init() { Object::init(); }
    QQuickPathItemStrokeFillParamsObject *obj;
};

} // namespace Heap
} // namespace QV4

struct QQuickPathItemJSPathPrototype : public QV4::Object
{
    V4_OBJECT2(QQuickPathItemJSPathPrototype, QV4::Object)
public:
    static QV4::Heap::QQuickPathItemJSPathPrototype *create(QV4::ExecutionEngine *engine)
    {
        QV4::Scope scope(engine);
        auto obj = engine->memoryManager->allocObject<QQuickPathItemJSPathPrototype>();
        QV4::Scoped<QQuickPathItemJSPathPrototype> o(scope, obj);

        o->defineDefaultProperty(QStringLiteral("clear"), method_clear, 0);
        o->defineDefaultProperty(QStringLiteral("moveTo"), method_moveTo, 0);
        o->defineDefaultProperty(QStringLiteral("lineTo"), method_lineTo, 0);
        o->defineDefaultProperty(QStringLiteral("quadTo"), method_quadTo, 0);
        o->defineDefaultProperty(QStringLiteral("cubicTo"), method_cubicTo, 0);
        o->defineDefaultProperty(QStringLiteral("arcTo"), method_arcTo, 0);

        return o->d();
    }

    static void method_clear(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_moveTo(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_lineTo(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_quadTo(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_cubicTo(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_arcTo(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
};

DEFINE_OBJECT_VTABLE(QQuickPathItemJSPathPrototype);

struct QQuickPathItemJSStrokeFillParamsPrototype : public QV4::Object
{
    V4_OBJECT2(QQuickPathItemJSStrokeFillParamsPrototype, QV4::Object)
public:
    static QV4::Heap::QQuickPathItemJSStrokeFillParamsPrototype *create(QV4::ExecutionEngine *engine)
    {
        QV4::Scope scope(engine);
        auto obj = engine->memoryManager->allocObject<QQuickPathItemJSStrokeFillParamsPrototype>();
        QV4::Scoped<QQuickPathItemJSStrokeFillParamsPrototype> o(scope, obj);

        o->defineDefaultProperty(QStringLiteral("clear"), method_clear, 0);

        return o->d();
    }

    static void method_clear(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
};

DEFINE_OBJECT_VTABLE(QQuickPathItemJSStrokeFillParamsPrototype);

struct QQuickPathItemJSPath : public QV4::Object
{
    V4_OBJECT2(QQuickPathItemJSPath, QV4::Object)
};

DEFINE_OBJECT_VTABLE(QQuickPathItemJSPath);

struct QQuickPathItemJSStrokeFillParams : public QV4::Object
{
    V4_OBJECT2(QQuickPathItemJSStrokeFillParams, QV4::Object)

    static void method_get_strokeColor(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_set_strokeColor(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_get_strokeWidth(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_set_strokeWidth(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_get_fillColor(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_set_fillColor(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_get_fillRule(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_set_fillRule(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_get_joinStyle(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_set_joinStyle(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_get_miterLimit(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_set_miterLimit(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_get_capStyle(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_set_capStyle(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_get_strokeStyle(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_set_strokeStyle(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_get_dashOffset(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_set_dashOffset(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_get_dashPattern(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_set_dashPattern(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_get_fillGradient(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
    static void method_set_fillGradient(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData);
};

DEFINE_OBJECT_VTABLE(QQuickPathItemJSStrokeFillParams);

void QQuickPathItemJSPathPrototype::method_clear(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSPath> r(scope, callData->thisObject.as<QQuickPathItemJSPath>());

    r->d()->obj->clear();

    scope.result = callData->thisObject.asReturnedValue();
}

void QQuickPathItemJSPathPrototype::method_moveTo(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSPath> r(scope, callData->thisObject.as<QQuickPathItemJSPath>());

    if (callData->argc >= 2) {
        QQuickPathItemPathObject *p = r->d()->obj;
        p->path.cmd.append(QQuickPathItemPath::MoveTo);
        p->path.coords.append(callData->args[0].toNumber());
        p->path.coords.append(callData->args[1].toNumber());
    }

    scope.result = callData->thisObject.asReturnedValue();
}

void QQuickPathItemJSPathPrototype::method_lineTo(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSPath> r(scope, callData->thisObject.as<QQuickPathItemJSPath>());

    if (callData->argc >= 2) {
        QQuickPathItemPathObject *p = r->d()->obj;
        p->path.cmd.append(QQuickPathItemPath::LineTo);
        p->path.coords.append(callData->args[0].toNumber());
        p->path.coords.append(callData->args[1].toNumber());
    }

    scope.result = callData->thisObject.asReturnedValue();
}

void QQuickPathItemJSPathPrototype::method_quadTo(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSPath> r(scope, callData->thisObject.as<QQuickPathItemJSPath>());

    if (callData->argc >= 4) {
        QQuickPathItemPathObject *p = r->d()->obj;
        p->path.cmd.append(QQuickPathItemPath::QuadTo);
        const QV4::Value *v = callData->args;
        p->path.coords.append(v[0].toNumber()); // cx
        p->path.coords.append(v[1].toNumber()); // cy
        p->path.coords.append(v[2].toNumber()); // x
        p->path.coords.append(v[3].toNumber()); // y
    }

    scope.result = callData->thisObject.asReturnedValue();
}

void QQuickPathItemJSPathPrototype::method_cubicTo(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSPath> r(scope, callData->thisObject.as<QQuickPathItemJSPath>());

    if (callData->argc >= 6) {
        QQuickPathItemPathObject *p = r->d()->obj;
        p->path.cmd.append(QQuickPathItemPath::CubicTo);
        const QV4::Value *v = callData->args;
        p->path.coords.append(v[0].toNumber()); // c1x
        p->path.coords.append(v[1].toNumber()); // c1y
        p->path.coords.append(v[2].toNumber()); // c2x
        p->path.coords.append(v[3].toNumber()); // c2y
        p->path.coords.append(v[4].toNumber()); // x
        p->path.coords.append(v[5].toNumber()); // y
    }

    scope.result = callData->thisObject.asReturnedValue();
}

void QQuickPathItemJSPathPrototype::method_arcTo(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSPath> r(scope, callData->thisObject.as<QQuickPathItemJSPath>());

    if (callData->argc >= 7) {
        QQuickPathItemPathObject *p = r->d()->obj;
        p->path.cmd.append(QQuickPathItemPath::ArcTo);
        const QV4::Value *v = callData->args;
        p->path.coords.append(v[0].toNumber()); // radiusX
        p->path.coords.append(v[1].toNumber()); // radiusY
        p->path.coords.append(v[2].toNumber()); // xAxisRotation
        p->path.coords.append(v[3].toNumber()); // x
        p->path.coords.append(v[4].toNumber()); // y
        p->path.coords.append(v[5].toNumber()); // sweepFlag
        p->path.coords.append(v[6].toNumber()); // largeArc
    }

    scope.result = callData->thisObject.asReturnedValue();
}

void QQuickPathItemJSStrokeFillParamsPrototype::method_clear(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    r->d()->obj->clear();

    scope.result = callData->thisObject.asReturnedValue();
}

extern QColor qt_color_from_string(const QV4::Value &name); // qquickcontext2d.cpp

static inline QString qt_color_string(const QColor &color)
{
    if (color.alpha() == 255)
        return color.name();
    QString alphaString = QString::number(color.alphaF(), 'f');
    while (alphaString.endsWith(QLatin1Char('0')))
        alphaString.chop(1);
    if (alphaString.endsWith(QLatin1Char('.')))
        alphaString += QLatin1Char('0');
    return QString::fromLatin1("rgba(%1, %2, %3, %4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(alphaString);
}

void QQuickPathItemJSStrokeFillParams::method_get_strokeColor(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    scope.result = QV4::Encode(scope.engine->newString(qt_color_string(r->d()->obj->sfp.strokeColor)));
}

void QQuickPathItemJSStrokeFillParams::method_set_strokeColor(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    QV4::ScopedValue value(scope, callData->argument(0));
    if (value->isString())
        r->d()->obj->sfp.strokeColor = qt_color_from_string(value);

    scope.result = QV4::Encode::undefined();
}

void QQuickPathItemJSStrokeFillParams::method_get_strokeWidth(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    scope.result = scope.engine->fromVariant(r->d()->obj->sfp.strokeWidth);
}

void QQuickPathItemJSStrokeFillParams::method_set_strokeWidth(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    QV4::ScopedValue value(scope, callData->argument(0));
    r->d()->obj->sfp.strokeWidth = value->toNumber();

    scope.result = QV4::Encode::undefined();
}

void QQuickPathItemJSStrokeFillParams::method_get_fillColor(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    scope.result = QV4::Encode(scope.engine->newString(qt_color_string(r->d()->obj->sfp.fillColor)));
}

void QQuickPathItemJSStrokeFillParams::method_set_fillColor(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    QV4::ScopedValue value(scope, callData->argument(0));
    if (value->isString())
        r->d()->obj->sfp.fillColor = qt_color_from_string(value);

    scope.result = QV4::Encode::undefined();
}

void QQuickPathItemJSStrokeFillParams::method_get_fillRule(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    scope.result = scope.engine->fromVariant(r->d()->obj->sfp.fillRule);
}

void QQuickPathItemJSStrokeFillParams::method_set_fillRule(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    QV4::ScopedValue value(scope, callData->argument(0));
    if (value->isInt32())
        r->d()->obj->sfp.fillRule = QQuickVisualPath::FillRule(value->integerValue());

    scope.result = QV4::Encode::undefined();
}

void QQuickPathItemJSStrokeFillParams::method_get_joinStyle(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    scope.result = scope.engine->fromVariant(r->d()->obj->sfp.joinStyle);
}

void QQuickPathItemJSStrokeFillParams::method_set_joinStyle(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    QV4::ScopedValue value(scope, callData->argument(0));
    if (value->isInt32())
        r->d()->obj->sfp.joinStyle = QQuickVisualPath::JoinStyle(value->integerValue());

    scope.result = QV4::Encode::undefined();
}

void QQuickPathItemJSStrokeFillParams::method_get_miterLimit(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    scope.result = scope.engine->fromVariant(r->d()->obj->sfp.miterLimit);
}

void QQuickPathItemJSStrokeFillParams::method_set_miterLimit(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    QV4::ScopedValue value(scope, callData->argument(0));
    r->d()->obj->sfp.miterLimit = value->toNumber();

    scope.result = QV4::Encode::undefined();
}

void QQuickPathItemJSStrokeFillParams::method_get_capStyle(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    scope.result = scope.engine->fromVariant(r->d()->obj->sfp.capStyle);
}

void QQuickPathItemJSStrokeFillParams::method_set_capStyle(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    QV4::ScopedValue value(scope, callData->argument(0));
    if (value->isInt32())
        r->d()->obj->sfp.capStyle = QQuickVisualPath::CapStyle(value->integerValue());

    scope.result = QV4::Encode::undefined();
}

void QQuickPathItemJSStrokeFillParams::method_get_strokeStyle(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    scope.result = scope.engine->fromVariant(r->d()->obj->sfp.strokeStyle);
}

void QQuickPathItemJSStrokeFillParams::method_set_strokeStyle(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    QV4::ScopedValue value(scope, callData->argument(0));
    if (value->isInt32())
        r->d()->obj->sfp.strokeStyle = QQuickVisualPath::StrokeStyle(value->integerValue());

    scope.result = QV4::Encode::undefined();
}

void QQuickPathItemJSStrokeFillParams::method_get_dashOffset(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    scope.result = scope.engine->fromVariant(r->d()->obj->sfp.dashOffset);
}

void QQuickPathItemJSStrokeFillParams::method_set_dashOffset(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    QV4::ScopedValue value(scope, callData->argument(0));
    r->d()->obj->sfp.dashOffset = value->toNumber();

    scope.result = QV4::Encode::undefined();
}

void QQuickPathItemJSStrokeFillParams::method_get_dashPattern(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    QV4::ScopedArrayObject a(scope, scope.engine->newArrayObject());
    QQuickPathItemStrokeFillParamsObject *p = r->d()->obj;
    a->arrayReserve(p->sfp.dashPattern.count());
    QV4::ScopedValue v(scope);
    for (int i = 0; i < p->sfp.dashPattern.count(); ++i)
        a->arrayPut(i, (v = scope.engine->fromVariant(p->sfp.dashPattern[i])));
    a->setArrayLengthUnchecked(p->sfp.dashPattern.count());

    scope.result = a.asReturnedValue();
}

void QQuickPathItemJSStrokeFillParams::method_set_dashPattern(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    QV4::ScopedValue value(scope, callData->argument(0));
    if (value->isObject()) {
        QV4::Scoped<QV4::ArrayObject> ao(scope, value);
       if (!!ao) {
            QQuickPathItemStrokeFillParamsObject *p = r->d()->obj;
            p->sfp.dashPattern.resize(ao->getLength());
            QV4::ScopedValue val(scope);
            for (int i = 0; i < p->sfp.dashPattern.count(); ++i) {
                val = ao->getIndexed(i);
                p->sfp.dashPattern[i] = val->toNumber();
            }
        }
    }

    scope.result = QV4::Encode::undefined();
}

void QQuickPathItemJSStrokeFillParams::method_get_fillGradient(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    scope.result = r->d()->obj->v4fillGradient.value();
}

void QQuickPathItemJSStrokeFillParams::method_set_fillGradient(const QV4::BuiltinFunction *, QV4::Scope &scope, QV4::CallData *callData)
{
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> r(scope, callData->thisObject.as<QQuickPathItemJSStrokeFillParams>());

    QV4::ScopedValue value(scope, callData->argument(0));
    QV4::Scoped<QV4::QObjectWrapper> qobjectWrapper(scope, value);
    if (!!qobjectWrapper) {
        if (QQuickPathGradient *grad = qobject_cast<QQuickPathGradient *>(qobjectWrapper->object())) {
            r->d()->obj->v4fillGradient.set(scope.engine, value);
            r->d()->obj->sfp.fillGradient = grad;
        }
    } else {
        r->d()->obj->v4fillGradient.set(scope.engine, nullptr);
        r->d()->obj->sfp.fillGradient = nullptr;
    }

    scope.result = QV4::Encode::undefined();
}

QQuickPathItemJSEngineData::QQuickPathItemJSEngineData(QV4::ExecutionEngine *v4)
{
    QV4::Scope scope(v4);

    QV4::ScopedObject proto(scope, QQuickPathItemJSPathPrototype::create(v4));
    pathProto = proto;

    proto = QV4::ScopedObject(scope, QQuickPathItemJSStrokeFillParamsPrototype::create(v4));

    proto->defineAccessorProperty(QStringLiteral("strokeColor"),
                                  QQuickPathItemJSStrokeFillParams::method_get_strokeColor,
                                  QQuickPathItemJSStrokeFillParams::method_set_strokeColor);
    proto->defineAccessorProperty(QStringLiteral("strokeWidth"),
                                  QQuickPathItemJSStrokeFillParams::method_get_strokeWidth,
                                  QQuickPathItemJSStrokeFillParams::method_set_strokeWidth);
    proto->defineAccessorProperty(QStringLiteral("fillColor"),
                                  QQuickPathItemJSStrokeFillParams::method_get_fillColor,
                                  QQuickPathItemJSStrokeFillParams::method_set_fillColor);
    proto->defineAccessorProperty(QStringLiteral("fillRule"),
                                  QQuickPathItemJSStrokeFillParams::method_get_fillRule,
                                  QQuickPathItemJSStrokeFillParams::method_set_fillRule);
    proto->defineAccessorProperty(QStringLiteral("joinStyle"),
                                  QQuickPathItemJSStrokeFillParams::method_get_joinStyle,
                                  QQuickPathItemJSStrokeFillParams::method_set_joinStyle);
    proto->defineAccessorProperty(QStringLiteral("miterLimit"),
                                  QQuickPathItemJSStrokeFillParams::method_get_miterLimit,
                                  QQuickPathItemJSStrokeFillParams::method_set_miterLimit);
    proto->defineAccessorProperty(QStringLiteral("capStyle"),
                                  QQuickPathItemJSStrokeFillParams::method_get_capStyle,
                                  QQuickPathItemJSStrokeFillParams::method_set_capStyle);
    proto->defineAccessorProperty(QStringLiteral("strokeStyle"),
                                  QQuickPathItemJSStrokeFillParams::method_get_strokeStyle,
                                  QQuickPathItemJSStrokeFillParams::method_set_strokeStyle);
    proto->defineAccessorProperty(QStringLiteral("dashOffset"),
                                  QQuickPathItemJSStrokeFillParams::method_get_dashOffset,
                                  QQuickPathItemJSStrokeFillParams::method_set_dashOffset);
    proto->defineAccessorProperty(QStringLiteral("dashPattern"),
                                  QQuickPathItemJSStrokeFillParams::method_get_dashPattern,
                                  QQuickPathItemJSStrokeFillParams::method_set_dashPattern);
    proto->defineAccessorProperty(QStringLiteral("fillGradient"),
                                  QQuickPathItemJSStrokeFillParams::method_get_fillGradient,
                                  QQuickPathItemJSStrokeFillParams::method_set_fillGradient);

    strokeFillParamsProto = proto;
}

void QQuickPathItemPathObject::setV4Engine(QV4::ExecutionEngine *engine)
{
    QQuickPathItemJSEngineData *ed = engineData(engine);
    QV4::Scope scope(engine);
    QV4::Scoped<QQuickPathItemJSPath> wrapper(scope, engine->memoryManager->allocObject<QQuickPathItemJSPath>());
    QV4::ScopedObject p(scope, ed->pathProto.value());
    wrapper->setPrototype(p);
    wrapper->d()->obj = this;
    m_v4value = wrapper;
}

void QQuickPathItemPathObject::clear()
{
    path = QQuickPathItemPath();
}

void QQuickPathItemStrokeFillParamsObject::clear()
{
    sfp = QQuickPathItemStrokeFillParams();
    if (!v4fillGradient.isNullOrUndefined())
        v4fillGradient.set(v4fillGradient.engine(), nullptr);
}

void QQuickPathItemStrokeFillParamsObject::setV4Engine(QV4::ExecutionEngine *engine)
{
    QQuickPathItemJSEngineData *ed = engineData(engine);
    QV4::Scope scope(engine);
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> wrapper(scope, engine->memoryManager->allocObject<QQuickPathItemJSStrokeFillParams>());
    QV4::ScopedObject p(scope, ed->strokeFillParamsProto.value());
    wrapper->setPrototype(p);
    wrapper->d()->obj = this;
    m_v4value = wrapper;
}

void QQuickPathItem::newPath(QQmlV4Function *args)
{
    QQuickPathItemPathObject *obj = new QQuickPathItemPathObject(this);
    obj->setV4Engine(QQmlEnginePrivate::get(qmlEngine(this))->v4engine());
    args->setReturnValue(obj->v4value());
}

void QQuickPathItem::newStrokeFillParams(QQmlV4Function *args)
{
    QQuickPathItemStrokeFillParamsObject *obj = new QQuickPathItemStrokeFillParamsObject(this);
    obj->setV4Engine(QQmlEnginePrivate::get(qmlEngine(this))->v4engine());
    args->setReturnValue(obj->v4value());
}

void QQuickPathItem::clearVisualPaths(QQmlV4Function *args)
{
    Q_UNUSED(args);
    Q_D(QQuickPathItem);
    d->jsData.paths.clear();
    d->jsData.sfp.clear();
}

void QQuickPathItem::commitVisualPaths(QQmlV4Function *args)
{
    Q_UNUSED(args);
    Q_D(QQuickPathItem);
    d->_q_visualPathChanged();
}

void QQuickPathItem::appendVisualPath(QQmlV4Function *args)
{
    if (args->length() < 2)
        return;

    Q_D(QQuickPathItem);
    QV4::Scope scope(args->v4engine());
    QV4::Scoped<QQuickPathItemJSPath> jsp(scope, (*args)[0]);
    QV4::Scoped<QQuickPathItemJSStrokeFillParams> jssfp(scope, (*args)[1]);

    const QQuickPathItemPath &path(jsp->d()->obj->path);
    const QQuickPathItemStrokeFillParams &sfp(jssfp->d()->obj->sfp);

    d->jsData.paths.append(path);
    d->jsData.sfp.append(sfp);
}

QT_END_NAMESPACE

#include "moc_qquickpathitem_p.cpp"
