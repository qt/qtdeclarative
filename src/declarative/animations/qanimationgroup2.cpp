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



QT_BEGIN_NAMESPACE

QAnimationGroup2::QAnimationGroup2(QDeclarativeAbstractAnimation *animation)
    :QAbstractAnimation2(animation)
{
    m_isGroup = true;
}

QAnimationGroup2::~QAnimationGroup2()
{
    foreach (QAbstractAnimation2* child, m_animations) {
        child->setGroup(0);
    }
    m_animations.clear();   //### can remove if setGroup handles this
}

QAbstractAnimation2 *QAnimationGroup2::animationAt(int index) const
{
    if (index < 0 || index >= m_animations.size()) {
        qWarning("QAnimationGroup2::animationAt: index is out of bounds");
        return 0;
    }

    return m_animations.at(index);
}

int QAnimationGroup2::animationCount() const
{
    return m_animations.size();
}

int QAnimationGroup2::indexOfAnimation(QAbstractAnimation2 *animation) const
{
    return m_animations.indexOf(animation);
}

void QAnimationGroup2::addAnimation(QAbstractAnimation2 *animation)
{
    insertAnimation(m_animations.count(), animation);
}

void QAnimationGroup2::insertAnimation(int index, QAbstractAnimation2 *animation)
{
    if (index < 0 || index > m_animations.size()) {
        qWarning("QAnimationGroup2::insertAnimation: index is out of bounds");
        return;
    }

    if (QAnimationGroup2 *oldGroup = animation->group())
        oldGroup->removeAnimation(animation);

    m_animations.insert(index, animation);
    animation->setGroup(this);
    animationInsertedAt(index);
}

void QAnimationGroup2::removeAnimation(QAbstractAnimation2 *animation)
{
    if (!animation) {
        qWarning("QAnimationGroup2::remove: cannot remove null animation");
        return;
    }
    int index = m_animations.indexOf(animation);
    if (index == -1) {
        qWarning("QAnimationGroup2::remove: animation is not part of this group");
        return;
    }

    takeAnimation(index);
}

QAbstractAnimation2 *QAnimationGroup2::takeAnimation(int index)
{
    if (index < 0 || index >= m_animations.size()) {
        qWarning("QAnimationGroup2::takeAnimation: no animation at index %d", index);
        return 0;
    }
    QAbstractAnimation2 *animation = m_animations.at(index);
    animation->setGroup(0);
    m_animations.removeAt(index);
    animationRemoved(index, animation);
    return animation;
}

void QAnimationGroup2::clear()
{
    //qDeleteAll(m_animations);
    foreach (QAbstractAnimation2* child, m_animations) {
        child->setGroup(0);
    }
    m_animations.clear();
    //TODO: other cleanup
}

bool QAnimationGroup2::isAnimationConnected(QAbstractAnimation2 *anim) const
{
    return m_uncontrolledFinishTime.contains(anim);
}
bool QAnimationGroup2::isUncontrolledAnimationFinished(QAbstractAnimation2 *anim) const
{
    return m_uncontrolledFinishTime.value(anim, -1) >= 0;
}

void QAnimationGroup2::disconnectUncontrolledAnimations()
{
    m_uncontrolledFinishTime.clear();
}

void QAnimationGroup2::connectUncontrolledAnimations()
{
    for (int i = 0; i < m_animations.size(); ++i) {
        QAbstractAnimation2 *animation = m_animations.at(i);
        if (animation->duration() == -1 || animation->loopCount() < 0) {
            m_uncontrolledFinishTime[animation] = -1;
        }
    }
}
void QAnimationGroup2::connectUncontrolledAnimation(QAbstractAnimation2 *anim)
{
    m_uncontrolledFinishTime[anim] = -1;
}

void QAnimationGroup2::disconnectUncontrolledAnimation(QAbstractAnimation2 *anim)
{
    m_uncontrolledFinishTime.remove(anim);
}

void QAnimationGroup2::uncontrolledAnimationFinished(QAbstractAnimation2* animation)
{
    Q_UNUSED(animation);
}

void QAnimationGroup2::animationRemoved(int index, QAbstractAnimation2 *)
{
    Q_UNUSED(index);
    if (m_animations.isEmpty()) {
        m_currentTime = 0;
        stop();
    }
}

QT_END_NAMESPACE
