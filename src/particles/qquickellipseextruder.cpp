// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickellipseextruder_p.h"
#include <qmath.h>
#include <qrandom.h>

QT_BEGIN_NAMESPACE
/*!
    \qmltype EllipseShape
    \nativetype QQuickEllipseExtruder
    \inqmlmodule QtQuick.Particles
    \ingroup qtquick-particles
    \inherits Shape
    \brief Represents an ellipse to other particle system elements.

    This shape can be used by Emitter subclasses and Affector subclasses to have
    them act upon an ellipse shaped area.
*/
QQuickEllipseExtruder::QQuickEllipseExtruder(QObject *parent) :
    QQuickParticleExtruder(parent)
  , m_fill(true)
{
}

/*!
    \qmlproperty bool QtQuick.Particles::EllipseShape::fill
    If fill is true the ellipse is filled; otherwise it is just a border.

    Default is true.
*/

QPointF QQuickEllipseExtruder::extrude(const QRectF & r)
{
    qreal theta = QRandomGenerator::global()->bounded(2 * M_PI);
    qreal mag = m_fill ? QRandomGenerator::global()->generateDouble() : 1;
    return QPointF(r.x() + r.width()/2 + mag * (r.width()/2) * qCos(theta),
                   r.y() + r.height()/2 + mag * (r.height()/2) * qSin(theta));
}

bool QQuickEllipseExtruder::contains(const QRectF &bounds, const QPointF &point)
{
    if (!bounds.contains(point))
        return false;

    QPointF relPoint(bounds.center() - point);
    qreal xa = relPoint.x()/bounds.width();
    qreal yb = relPoint.y()/bounds.height();
    return  (xa * xa + yb * yb) < 0.25;
}

QT_END_NAMESPACE

#include "moc_qquickellipseextruder_p.cpp"
