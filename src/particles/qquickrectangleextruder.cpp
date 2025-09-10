// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickrectangleextruder_p.h"
#include <QRandomGenerator>

QT_BEGIN_NAMESPACE

/*!
    \qmltype RectangleShape
    \nativetype QQuickRectangleExtruder
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
