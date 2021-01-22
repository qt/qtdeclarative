/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qquickparticleextruder_p.h"
#include <QRandomGenerator>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ParticleExtruder
    \instantiates QQuickParticleExtruder
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
