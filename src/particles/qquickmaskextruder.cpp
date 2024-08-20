// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmaskextruder_p.h"
#include <QtQml/qqml.h>
#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlcontext.h>

#include <QImage>
#include <QDebug>
#include <QRandomGenerator>

QT_BEGIN_NAMESPACE
/*!
    \qmltype MaskShape
    \nativetype QQuickMaskExtruder
    \inqmlmodule QtQuick.Particles
    \inherits Shape
    \brief For representing an image as a shape to affectors and emitters.
    \ingroup qtquick-particles

*/
/*!
    \qmlproperty url QtQuick.Particles::MaskShape::source

    The image to use as the mask. Areas with non-zero opacity
    will be considered inside the shape.
*/


QQuickMaskExtruder::QQuickMaskExtruder(QObject *parent) :
    QQuickParticleExtruder(parent)
  , m_lastWidth(-1)
  , m_lastHeight(-1)
{
}

void QQuickMaskExtruder::setSource(const QUrl &arg)
{
    if (m_source != arg) {
        m_source = arg;

        m_lastHeight = -1;//Trigger reset
        m_lastWidth = -1;
        emit sourceChanged(m_source);
        startMaskLoading();
    }
}

void QQuickMaskExtruder::startMaskLoading()
{
    m_pix.clear(this);
    if (m_source.isEmpty())
        return;
    const QQmlContext *context = qmlContext(this);
    m_pix.load(context->engine(), context->resolvedUrl(m_source));
    if (m_pix.isLoading())
        m_pix.connectFinished(this, SLOT(finishMaskLoading()));
    else
        finishMaskLoading();
}

void QQuickMaskExtruder::finishMaskLoading()
{
    if (m_pix.isError())
        qmlWarning(this) << m_pix.error();
}

QPointF QQuickMaskExtruder::extrude(const QRectF &r)
{
    ensureInitialized(r);
    if (!m_mask.size() || m_img.isNull())
        return r.topLeft();
    const QPointF p = m_mask[QRandomGenerator::global()->bounded(m_mask.size())];
    //### Should random sub-pixel positioning be added?
    return p + r.topLeft();
}

bool QQuickMaskExtruder::contains(const QRectF &bounds, const QPointF &point)
{
    ensureInitialized(bounds);//###Current usage patterns WILL lead to different bounds/r calls. Separate list?
    if (m_img.isNull())
        return false;

    QPointF pt = point - bounds.topLeft();
    QPoint p(pt.x() * m_img.width() / bounds.width(),
             pt.y() * m_img.height() / bounds.height());
    return m_img.rect().contains(p) && (m_img.pixel(p) & 0xff000000);
}

void QQuickMaskExtruder::ensureInitialized(const QRectF &rf)
{
    // Convert to integer coords to avoid comparing floats and ints which would
    // often result in rounding errors.
    QRect r = rf.toRect();
    if (m_lastWidth == r.width() && m_lastHeight == r.height())
        return;//Same as before
    if (!m_pix.isReady())
        return;
    m_lastWidth = r.width();
    m_lastHeight = r.height();

    m_mask.clear();

    m_img = m_pix.image();
    // Image will in all likelyhood be in this format already, so
    // no extra memory or conversion takes place
    if (m_img.format() != QImage::Format_ARGB32 && m_img.format() != QImage::Format_ARGB32_Premultiplied)
        m_img = std::move(m_img).convertToFormat(QImage::Format_ARGB32_Premultiplied);

    // resample on the fly using 16-bit
    int sx = (m_img.width() << 16) / r.width();
    int sy = (m_img.height() << 16) / r.height();
    int w = r.width();
    int h = r.height();
    for (int y=0; y<h; ++y) {
        const uint *sl = (const uint *) m_img.constScanLine((y * sy) >> 16);
        for (int x=0; x<w; ++x) {
            if (sl[(x * sx) >> 16] & 0xff000000)
                m_mask << QPointF(x, y);
        }
    }
}
QT_END_NAMESPACE

#include "moc_qquickmaskextruder_p.cpp"
