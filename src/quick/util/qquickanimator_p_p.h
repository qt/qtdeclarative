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

#ifndef QQUICKANIMATOR_P_P_H
#define QQUICKANIMATOR_P_P_H

#include "qquickanimator_p.h"
#include "qquickanimation_p_p.h"
#include <QtQuick/qquickitem.h>

QT_BEGIN_NAMESPACE

class QQuickAnimatorJob;

class QQuickAnimatorPrivate : public QQuickAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QQuickAnimator)
public:
    QQuickAnimatorPrivate()
        : target(0)
        , duration(250)
        , from(0)
        , to(0)
        , isFromDefined(false)
        , isToDefined(false)
    {
    }

    QPointer<QQuickItem> target;
    int duration;
    QEasingCurve easing;
    qreal from;
    qreal to;

    uint isFromDefined : 1;
    uint isToDefined : 1;

    void apply(QQuickAnimatorJob *job, const QString &propertyName, QQuickStateActions &actions, QQmlProperties &modified);
};

class QQuickRotationAnimatorPrivate : public QQuickAnimatorPrivate
{
public:
    QQuickRotationAnimatorPrivate()
        : direction(QQuickRotationAnimator::Numerical)
    {
    }
    QQuickRotationAnimator::RotationDirection direction;
};

class QQuickUniformAnimatorPrivate : public QQuickAnimatorPrivate
{
public:
    QString uniform;
};

QT_END_NAMESPACE

#endif // QQUICKANIMATOR_P_P_H
