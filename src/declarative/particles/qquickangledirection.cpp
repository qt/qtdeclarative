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

#include "qquickangledirection_p.h"
#include <cmath>
QT_BEGIN_NAMESPACE
const qreal CONV = 0.017453292519943295;
/*!
    \qmlclass AngleDirection QQuickAngleDirection
    \inqmlmodule QtQuick.Particles 2
    \inherits Direction
    \brief The AngleDirection element allows you to specify a direction that varies in angle

    The AngledDirection element allows both the specification of a direction by angle and magnitude,
    as well as varying the parameters by angle or magnitude.
*/
/*!
    \qmlproperty real QtQuick.Particles2::AngleDirection::angle
    This property specifies the base angle for the direction.
    The angle of this direction will vary by no more than angleVariation
    from this angle.

    Angle is specified by degrees clockwise from straight right.

    The default value is zero.
*/
/*!
    \qmlproperty real QtQuick.Particles2::AngleDirection::magnitude
    This property specifies the base magnitude for the direction.
    The magnitude of this direction will vary by no more than magnitudeVariation
    from this magnitude.

    Magnitude is specified in units of pixels per second.

    The default value is zero.
*/
/*!
    \qmlproperty real QtQuick.Particles2::AngleDirection::angleVariation
    This property specifies the maximum angle variation for the direction.
    The angle of the direction will vary by up to angleVariation clockwise
    and anticlockwise from the value specified in angle.

    Angle is specified by degrees clockwise from straight right.

    The default value is zero.
*/
/*!
    \qmlproperty real QtQuick.Particles2::AngleDirection::magnitudeVariation
    This property specifies the base magnitude for the direction.
    The magnitude of this direction will vary by no more than magnitudeVariation
    from the base magnitude.

    Magnitude is specified in units of pixels per second.

    The default value is zero.
*/
QQuickAngleDirection::QQuickAngleDirection(QObject *parent) :
    QQuickDirection(parent)
  , m_angle(0)
  , m_magnitude(0)
  , m_angleVariation(0)
  , m_magnitudeVariation(0)
{

}

const QPointF QQuickAngleDirection::sample(const QPointF &from)
{
    Q_UNUSED(from);
    QPointF ret;
    qreal theta = m_angle*CONV - m_angleVariation*CONV + rand()/float(RAND_MAX) * m_angleVariation*CONV * 2;
    qreal mag = m_magnitude- m_magnitudeVariation + rand()/float(RAND_MAX) * m_magnitudeVariation * 2;
    ret.setX(mag * cos(theta));
    ret.setY(mag * sin(theta));
    return ret;
}

QT_END_NAMESPACE
