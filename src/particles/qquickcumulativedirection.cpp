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

#include "qquickcumulativedirection_p.h"
QT_BEGIN_NAMESPACE

/*!
    \qmltype CumulativeDirection
    \instantiates QQuickCumulativeDirection
    \inqmlmodule QtQuick.Particles
    \inherits Direction
    \brief For specifying a direction made of other directions.
    \ingroup qtquick-particles

    The CumulativeDirection element will act as a direction that sums the directions within it.
*/
QQuickCumulativeDirection::QQuickCumulativeDirection(QObject *parent):QQuickDirection(parent)
{
}

QQmlListProperty<QQuickDirection> QQuickCumulativeDirection::directions()
{
    return QQmlListProperty<QQuickDirection>(this, &m_directions);//TODO: Proper list property
}

QPointF QQuickCumulativeDirection::sample(const QPointF &from)
{
    QPointF ret;
    foreach (QQuickDirection* dir, m_directions)
        ret += dir->sample(from);
    return ret;
}

QT_END_NAMESPACE

#include "moc_qquickcumulativedirection_p.cpp"
