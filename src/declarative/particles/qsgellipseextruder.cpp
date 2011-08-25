/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgellipseextruder_p.h"
#include <cmath>
QT_BEGIN_NAMESPACE
/*!
    \qmlclass EllipseShape QSGEllipseExtruder
    \inqmlmodule QtQuick.Particles 2
    \inherits Shape
    \brief The EllipseShape represents an ellipse to other particle system elements

    This shape can be used by Emitter subclasses and Affector subclasses to have
    them act upon an ellipse shaped area.
*/
QSGEllipseExtruder::QSGEllipseExtruder(QObject *parent) :
    QSGParticleExtruder(parent)
  , m_fill(true)
{
}

/*!
    \qmlproperty bool QtQuick.Particles2::EllipseShape::fill
    If fill is true the ellipse is filled; otherwise it is just a border.

    Default is true.
*/

QPointF QSGEllipseExtruder::extrude(const QRectF & r)
{
    qreal theta = ((qreal)rand()/RAND_MAX) * 6.2831853071795862;
    qreal mag = m_fill ? ((qreal)rand()/RAND_MAX) : 1;
    return QPointF(r.x() + r.width()/2 + mag * (r.width()/2) * cos(theta),
                   r.y() + r.height()/2 + mag * (r.height()/2) * sin(theta));
}

bool QSGEllipseExtruder::contains(const QRectF &bounds, const QPointF &point)
{
    return bounds.contains(point);//TODO: Ellipse
}

QT_END_NAMESPACE
