/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "private/qanimationgroup2_p.h"
#include <QtCore/qdebug.h>
#include <QtCore/qcoreevent.h>
#include "private/qanimationgroup2_p_p.h"



QT_BEGIN_NAMESPACE

QAnimationGroup2::QAnimationGroup2(QDeclarativeAbstractAnimation *animation)
    :QAbstractAnimation2(new QAnimationGroup2Private, animation)
{
}
QAnimationGroup2::QAnimationGroup2(QAnimationGroup2Private *dd, QDeclarativeAbstractAnimation *animation)
    :QAbstractAnimation2(dd, animation)
{
}

QAnimationGroup2::~QAnimationGroup2()
{
}

QAbstractAnimation2 *QAnimationGroup2::animationAt(int index) const
{
    if (index < 0 || index >= d_func()->animations.size()) {
        qWarning("QAnimationGroup2::animationAt: index is out of bounds");
        return 0;
    }

    return d_func()->animations.at(index);
}

int QAnimationGroup2::animationCount() const
{
    return d_func()->animations.size();
}

int QAnimationGroup2::indexOfAnimation(QAbstractAnimation2 *animation) const
{
    return d_func()->animations.indexOf(animation);
}

void QAnimationGroup2::addAnimation(QAbstractAnimation2 *animation)
{
    insertAnimation(d_func()->animations.count(), animation);
}

void QAnimationGroup2::insertAnimation(int index, QAbstractAnimation2 *animation)
{
    if (index < 0 || index > d_func()->animations.size()) {
        qWarning("QAnimationGroup2::insertAnimation: index is out of bounds");
        return;
    }

    if (QAnimationGroup2 *oldGroup = animation->group())
        oldGroup->removeAnimation(animation);

    d_func()->animations.insert(index, animation);
    QAbstractAnimation2Private::get(animation)->group = this;
//    animation->setParent(this);
    d_func()->animationInsertedAt(index);
}

void QAnimationGroup2::removeAnimation(QAbstractAnimation2 *animation)
{
    if (!animation) {
        qWarning("QAnimationGroup2::remove: cannot remove null animation");
        return;
    }
    int index = d_func()->animations.indexOf(animation);
    if (index == -1) {
        qWarning("QAnimationGroup2::remove: animation is not part of this group");
        return;
    }

    takeAnimation(index);
}

QAbstractAnimation2 *QAnimationGroup2::takeAnimation(int index)
{
    if (index < 0 || index >= d_func()->animations.size()) {
        qWarning("QAnimationGroup2::takeAnimation: no animation at index %d", index);
        return 0;
    }
    QAbstractAnimation2 *animation = d_func()->animations.at(index);
    QAbstractAnimation2Private::get(animation)->group = 0;
    d_func()->animations.removeAt(index);
//    animation->setParent(0);
    d_func()->animationRemoved(index, animation);
    return animation;
}

void QAnimationGroup2::clear()
{
    qDeleteAll(d_func()->animations);
}

bool QAnimationGroup2Private::isAnimationConnected(QAbstractAnimation2 *anim) const
{
    return uncontrolledFinishTime.contains(anim);
}
bool QAnimationGroup2Private::isUncontrolledAnimationFinished(QAbstractAnimation2 *anim) const
{
    return uncontrolledFinishTime.value(anim, -1) >= 0;
}

void QAnimationGroup2Private::disconnectUncontrolledAnimations()
{
    uncontrolledFinishTime.clear();
}

void QAnimationGroup2Private::connectUncontrolledAnimations()
{
    for (int i = 0; i < animations.size(); ++i) {
        QAbstractAnimation2 *animation = animations.at(i);
        if (animation->duration() == -1 || animation->loopCount() < 0) {
            uncontrolledFinishTime[animation] = -1;
        }
    }
}
void QAnimationGroup2Private::connectUncontrolledAnimation(QAbstractAnimation2 *anim)
{
    uncontrolledFinishTime[anim] = -1;
}

void QAnimationGroup2Private::disconnectUncontrolledAnimation(QAbstractAnimation2 *anim)
{
    uncontrolledFinishTime.remove(anim);
}

void QAnimationGroup2::uncontrolledAnimationFinished(QAbstractAnimation2* animation)
{
    Q_UNUSED(animation);
}

void QAnimationGroup2Private::animationRemoved(int index, QAbstractAnimation2 *)
{
    Q_UNUSED(index);
    if (animations.isEmpty()) {
        currentTime = 0;
        q->stop();
    }
}

QT_END_NAMESPACE
