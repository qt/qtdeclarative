// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
