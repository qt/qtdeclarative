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

#include "qquickpointdirection_p.h"
#include <stdlib.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype PointDirection
    \instantiates QQuickPointDirection
    \inqmlmodule QtQuick.Particles 2
    \ingroup qtquick-particles
    \inherits Direction
    \brief For specifying a direction that varies in x and y components

    The PointDirection element allows both the specification of a direction by x and y components,
    as well as varying the parameters by x or y component.
*/
/*!
    \qmlproperty real QtQuick.Particles2::PointDirection::x
*/
/*!
    \qmlproperty real QtQuick.Particles2::PointDirection::y
*/
/*!
    \qmlproperty real QtQuick.Particles2::PointDirection::xVariation
*/
/*!
    \qmlproperty real QtQuick.Particles2::PointDirection::yVariation
*/

QQuickPointDirection::QQuickPointDirection(QObject *parent) :
    QQuickDirection(parent)
  , m_x(0)
  , m_y(0)
  , m_xVariation(0)
  , m_yVariation(0)
{
}

const QPointF QQuickPointDirection::sample(const QPointF &)
{
    QPointF ret;
    ret.setX(m_x - m_xVariation + rand() / float(RAND_MAX) * m_xVariation * 2);
    ret.setY(m_y - m_yVariation + rand() / float(RAND_MAX) * m_yVariation * 2);
    return ret;
}

QT_END_NAMESPACE
