/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKSMOOTHEDANIMATION_H
#define QQUICKSMOOTHEDANIMATION_H

#include <qqml.h>
#include "qquickanimation_p.h"

#include <QtCore/qobject.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQmlProperty;
class QQuickSmoothedAnimationPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickSmoothedAnimation : public QQuickNumberAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickSmoothedAnimation)
    Q_ENUMS(ReversingMode)

    Q_PROPERTY(qreal velocity READ velocity WRITE setVelocity NOTIFY velocityChanged)
    Q_PROPERTY(ReversingMode reversingMode READ reversingMode WRITE setReversingMode NOTIFY reversingModeChanged)
    Q_PROPERTY(qreal maximumEasingTime READ maximumEasingTime WRITE setMaximumEasingTime NOTIFY maximumEasingTimeChanged)

public:
    enum ReversingMode { Eased, Immediate, Sync };

    QQuickSmoothedAnimation(QObject *parent = 0);
    ~QQuickSmoothedAnimation();

    ReversingMode reversingMode() const;
    void setReversingMode(ReversingMode);

    virtual int duration() const;
    virtual void setDuration(int);

    qreal velocity() const;
    void setVelocity(qreal);

    int maximumEasingTime() const;
    void setMaximumEasingTime(int);

    virtual QAbstractAnimationJob* transition(QQuickStateActions &actions,
                            QQmlProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = 0);
Q_SIGNALS:
    void velocityChanged();
    void reversingModeChanged();
    void maximumEasingTimeChanged();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickSmoothedAnimation)

QT_END_HEADER

#endif // QQUICKSMOOTHEDANIMATION_H
