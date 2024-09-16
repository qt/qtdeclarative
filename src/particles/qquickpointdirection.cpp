// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpointdirection_p.h"
#include <QRandomGenerator>

QT_BEGIN_NAMESPACE

/*!
    \qmltype PointDirection
    \nativetype QQuickPointDirection
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
