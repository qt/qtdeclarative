// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qquickparticleextruder_p.h"
#include <QRandomGenerator>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ParticleExtruder
    \nativetype QQuickParticleExtruder
    \inqmlmodule QtQuick.Particles
    \brief For specifying an area for affectors and emitters.
    \ingroup qtquick-particles

    The base class is just a rectangle.
*/

QQuickParticleExtruder::QQuickParticleExtruder(QObject *parent) :
    QObject(parent)
{
}

QPointF QQuickParticleExtruder::extrude(const QRectF &rect)
{
    return QPointF(QRandomGenerator::global()->generateDouble() * rect.width() + rect.x(),
                   QRandomGenerator::global()->generateDouble() * rect.height() + rect.y());
}

bool QQuickParticleExtruder::contains(const QRectF &bounds, const QPointF &point)
{
    return bounds.contains(point);
}

QT_END_NAMESPACE

#include "moc_qquickparticleextruder_p.cpp"
