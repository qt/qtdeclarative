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

#include "qquickrectangleextruder_p.h"
#include <QRandomGenerator>

QT_BEGIN_NAMESPACE

/*!
    \qmltype RectangleShape
    \instantiates QQuickRectangleExtruder
    \inqmlmodule QtQuick.Particles
    \brief For specifying an area for affectors and emitter.
    \ingroup qtquick-particles

    Just a rectangle.
*/

QQuickRectangleExtruder::QQuickRectangleExtruder(QObject *parent) :
    QQuickParticleExtruder(parent), m_fill(true)
{
}

QPointF QQuickRectangleExtruder::extrude(const QRectF &rect)
{
    if (m_fill)
        return QPointF(QRandomGenerator::global()->generateDouble() * rect.width() + rect.x(),
                       QRandomGenerator::global()->generateDouble() * rect.height() + rect.y());
    int side = QRandomGenerator::global()->bounded(4);
    switch (side){//TODO: Doesn't this overlap the corners?
    case 0:
        return QPointF(rect.x(),
                       QRandomGenerator::global()->generateDouble() * rect.height() + rect.y());
    case 1:
        return QPointF(rect.width() + rect.x(),
                       QRandomGenerator::global()->generateDouble() * rect.height() + rect.y());
    case 2:
        return QPointF(QRandomGenerator::global()->generateDouble() * rect.width() + rect.x(),
                       rect.y());
    default:
        return QPointF(QRandomGenerator::global()->generateDouble() * rect.width() + rect.x(),
                       rect.height() + rect.y());
    }
}

bool QQuickRectangleExtruder::contains(const QRectF &bounds, const QPointF &point)
{
    return bounds.contains(point);
}

QT_END_NAMESPACE

#include "moc_qquickrectangleextruder_p.cpp"
