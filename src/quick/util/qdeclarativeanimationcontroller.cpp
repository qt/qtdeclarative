/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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
#include "qdeclarativeanimationcontroller_p.h"
#include <QtDeclarative/qdeclarativeinfo.h>
#include <private/qdeclarativeengine_p.h>

QT_BEGIN_NAMESPACE


class QDeclarativeAnimationControllerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeAnimationController)
public:
    QDeclarativeAnimationControllerPrivate()
        : progress(0.0), animation(0), animationInstance(0), finalized(false) {}

    qreal progress;
    QDeclarativeAbstractAnimation *animation;
    QAbstractAnimation2Pointer animationInstance;
    bool finalized:1;

};

/*!
    \qmlclass AnimationController QDeclarativeAnimationController
    \inqmlmodule QtQuick 2
    \ingroup qml-animation-transition
    \brief The AnimationController element allows you to control animations manually.

    Normally animations are driven by an internal timer, but the AnimationController
    allows the given \a animation to be driven by a \a progress value explicitly.
*/


QDeclarativeAnimationController::QDeclarativeAnimationController(QObject *parent)
: QObject(*(new QDeclarativeAnimationControllerPrivate), parent)
{
}

QDeclarativeAnimationController::~QDeclarativeAnimationController()
{
}

/*!
    \qmlproperty real QtQuick2::AnimationController::progress
    This property holds the animation progress value.

    The valid \c progress value is 0.0 to 1.0, setting values less than 0 will be converted to 0,
    setting values great than 1 will be converted to 1.
*/
qreal QDeclarativeAnimationController::progress() const
{
    Q_D(const QDeclarativeAnimationController);
    return d->progress;
}

void QDeclarativeAnimationController::setProgress(qreal progress)
{
    Q_D(QDeclarativeAnimationController);
    progress = qBound(qreal(0), progress, qreal(1));

    if (progress != d->progress) {
        d->progress = progress;
        updateProgress();
        emit progressChanged();
    }
}

/*!
    \qmlproperty real QtQuick2::AnimationController::animation
    \default

    This property holds the animation to be controlled by the AnimationController.

    Note:An animation controlled by AnimationController will always have its
         \c running and \c paused properties set to true. It can not be manually
         started or stopped (much like an animation in a Behavior can not be manually started or stopped).
*/
QDeclarativeAbstractAnimation *QDeclarativeAnimationController::animation() const
{
    Q_D(const QDeclarativeAnimationController);
    return d->animation;
}

void QDeclarativeAnimationController::classBegin()
{
    QDeclarativeEnginePrivate *engPriv = QDeclarativeEnginePrivate::get(qmlEngine(this));
    engPriv->registerFinalizeCallback(this, this->metaObject()->indexOfSlot("componentFinalized()"));
}


void QDeclarativeAnimationController::setAnimation(QDeclarativeAbstractAnimation *animation)
{
    Q_D(QDeclarativeAnimationController);

    if (animation != d->animation) {
        if (animation) {
            if (animation->userControlDisabled()) {
                qmlInfo(this) << "QDeclarativeAnimationController::setAnimation: the animation is controlled by others, can't be used in AnimationController.";
                return;
            }
            animation->setDisableUserControl();
        }

        if (d->animation)
            d->animation->setEnableUserControl();

        d->animation = animation;
        reload();
        emit animationChanged();
    }
}

/*!
    \qmlmethod QtQuick2::AnimationController::reload()
    \brief Reloads the animation properties.

    If the animation properties changed, calling this method to reload the animation definations.
*/
void QDeclarativeAnimationController::reload()
{
    Q_D(QDeclarativeAnimationController);
    if (!d->finalized)
        return;

    if (!d->animation) {
        d->animationInstance = 0;
    } else {
        QDeclarativeStateActions actions;
        QDeclarativeProperties properties;
        d->animationInstance = d->animation->transition(actions, properties, QDeclarativeAbstractAnimation::Forward);
        d->animationInstance->setLoopCount(1);
        d->animationInstance->start();
        d->animationInstance->pause();
        updateProgress();
    }
}

void QDeclarativeAnimationController::updateProgress()
{
    Q_D(QDeclarativeAnimationController);
    if (!d->animationInstance)
        return;

    d->animationInstance->start();
    QDeclarativeAnimationTimer::unregisterAnimation(d->animationInstance);
    d->animationInstance->setCurrentTime(d->progress * d->animationInstance->duration());
}

void QDeclarativeAnimationController::componentFinalized()
{
    Q_D(QDeclarativeAnimationController);
    d->finalized = true;
    reload();
}


QT_END_NAMESPACE


