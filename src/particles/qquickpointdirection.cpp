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

#include "qquickpointdirection_p.h"
#include <QRandomGenerator>

QT_BEGIN_NAMESPACE

/*!
    \qmltype PointDirection
    \instantiates QQuickPointDirection
    \inqmlmodule QtQuick.Particles
    \ingroup qtquick-particles
    \inherits Direction
    \brief For specifying a direction that varies in x and y components.

    The PointDirection element allows both the specification of a direction by x and y components,
    as well as varying the parameters by x or y component.
*/
/*!
    \qmlproperty real QtQuick.Particles::PointDirection::x
*/
/*!
    \qmlproperty real QtQuick.Particles::PointDirection::y
*/
/*!
    \qmlproperty real QtQuick.Particles::PointDirection::xVariation
*/
/*!
    \qmlproperty real QtQuick.Particles::PointDirection::yVariation
*/

QQuickPointDirection::QQuickPointDirection(QObject *parent) :
    QQuickDirection(parent)
  , m_x(0)
  , m_y(0)
  , m_xVariation(0)
  , m_yVariation(0)
{
}

QPointF QQuickPointDirection::sample(const QPointF &)
{
    QPointF ret;
    ret.setX(m_x - m_xVariation + QRandomGenerator::global()->generateDouble() * m_xVariation * 2);
    ret.setY(m_y - m_yVariation + QRandomGenerator::global()->generateDouble() * m_yVariation * 2);
    return ret;
}

QT_END_NAMESPACE

#include "moc_qquickpointdirection_p.cpp"
