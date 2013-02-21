/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickellipseextruder_p.h"
#include <stdlib.h>
#include <cmath>

#ifdef Q_OS_QNX
#include <math.h>
#endif

QT_BEGIN_NAMESPACE
/*!
    \qmltype EllipseShape
    \instantiates QQuickEllipseExtruder
    \inqmlmodule QtQuick.Particles 2
    \ingroup qtquick-particles
    \inherits Shape
    \brief Represents an ellipse to other particle system elements

    This shape can be used by Emitter subclasses and Affector subclasses to have
    them act upon an ellipse shaped area.
*/
QQuickEllipseExtruder::QQuickEllipseExtruder(QObject *parent) :
    QQuickParticleExtruder(parent)
  , m_fill(true)
{
}

/*!
    \qmlproperty bool QtQuick.Particles2::EllipseShape::fill
    If fill is true the ellipse is filled; otherwise it is just a border.

    Default is true.
*/

QPointF QQuickEllipseExtruder::extrude(const QRectF & r)
{
    qreal theta = ((qreal)rand()/RAND_MAX) * 6.2831853071795862;
    qreal mag = m_fill ? ((qreal)rand()/RAND_MAX) : 1;
    return QPointF(r.x() + r.width()/2 + mag * (r.width()/2) * cos(theta),
                   r.y() + r.height()/2 + mag * (r.height()/2) * sin(theta));
}

bool QQuickEllipseExtruder::contains(const QRectF &bounds, const QPointF &point)
{
    return bounds.contains(point);//TODO: Ellipse
}

QT_END_NAMESPACE
