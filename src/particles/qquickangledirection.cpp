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

#include "qquickangledirection_p.h"
#include <stdlib.h>
#include <cmath>
#ifdef Q_OS_QNX
#include <math.h>
#endif
QT_BEGIN_NAMESPACE
const qreal CONV = 0.017453292519943295;
/*!
    \qmltype AngleDirection
    \instantiates QQuickAngleDirection
    \inqmlmodule QtQuick.Particles 2
    \ingroup qtquick-particles
    \inherits Direction
    \brief For specifying a direction that varies in angle

    The AngledDirection element allows both the specification of a direction by angle and magnitude,
    as well as varying the parameters by angle or magnitude.
*/
/*!
    \qmlproperty real QtQuick.Particles::AngleDirection::angle
    This property specifies the base angle for the direction.
    The angle of this direction will vary by no more than angleVariation
    from this angle.

    Angle is specified by degrees clockwise from straight right.

    The default value is zero.
*/
/*!
    \qmlproperty real QtQuick.Particles::AngleDirection::magnitude
    This property specifies the base magnitude for the direction.
    The magnitude of this direction will vary by no more than magnitudeVariation
    from this magnitude.

    Magnitude is specified in units of pixels per second.

    The default value is zero.
*/
/*!
    \qmlproperty real QtQuick.Particles::AngleDirection::angleVariation
    This property specifies the maximum angle variation for the direction.
    The angle of the direction will vary by up to angleVariation clockwise
    and anticlockwise from the value specified in angle.

    Angle is specified by degrees clockwise from straight right.

    The default value is zero.
*/
/*!
    \qmlproperty real QtQuick.Particles::AngleDirection::magnitudeVariation
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
