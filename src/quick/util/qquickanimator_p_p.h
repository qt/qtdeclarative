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

#ifndef QQUICKANIMATOR_P_P_H
#define QQUICKANIMATOR_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

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
        , fromIsDefined(false)
        , toIsDefined(false)
    {
    }

    QPointer<QQuickItem> target;
    int duration;
    QEasingCurve easing;
    qreal from;
    qreal to;

    uint fromIsDefined : 1;
    uint toIsDefined : 1;

    void apply(QQuickAnimatorJob *job, const QString &propertyName, QQuickStateActions &actions, QQmlProperties &modified, QObject *defaultTarget);
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
