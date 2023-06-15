// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qquicklineextruder_p.h"
#include <QRandomGenerator>
#include <cmath>

/*!
    \qmltype LineShape
    \instantiates QQuickLineExtruder
    \inqmlmodule QtQuick.Particles
    \inherits ParticleExtruder
    \brief Represents a line for affectors and emitters.
    \ingroup qtquick-particles

*/

/*!
    \qmlproperty bool QtQuick.Particles::LineShape::mirrored

    By default, the line goes from (0,0) to (width, height) of the item that
    this shape is being applied to.

    If mirrored is set to true, this will be mirrored along the y axis.
    The line will then go from (0,height) to (width, 0).
*/

QQuickLineExtruder::QQuickLineExtruder(QObject *parent) :
    QQuickParticleExtruder(parent), m_mirrored(false)
{
}

QPointF QQuickLineExtruder::extrude(const QRectF &r)
{
    qreal x,y;
    if (!r.height()){
        x = r.width() * QRandomGenerator::global()->generateDouble();
        y = 0;
    }else{
        y = r.height() * QRandomGenerator::global()->generateDouble();
        if (!r.width()){
            x = 0;
        }else{
            x = r.width()/r.height() * y;
            if (m_mirrored)
                x = r.width() - x;
        }
    }
    return QPointF(x,y);
}

#include "moc_qquicklineextruder_p.cpp"
