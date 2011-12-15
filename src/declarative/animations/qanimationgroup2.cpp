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

#include "private/qanimationgroup2_p.h"
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QAnimationGroup2::QAnimationGroup2()
    : QAbstractAnimation2(), m_firstChild(0), m_lastChild(0)
{
    m_isGroup = true;
}

QAnimationGroup2::~QAnimationGroup2()
{
    while (firstChild() != 0)
        delete firstChild();
}

void QAnimationGroup2::topLevelAnimationLoopChanged()
{
    for (QAbstractAnimation2 *animation = firstChild(); animation; animation = animation->nextSibling())
        animation->topLevelAnimationLoopChanged();
}

void QAnimationGroup2::appendAnimation(QAbstractAnimation2 *animation)
{
    if (QAnimationGroup2 *oldGroup = animation->m_group)
        oldGroup->removeAnimation(animation);

    Q_ASSERT(!previousSibling() && !nextSibling());

    if (m_lastChild)
        m_lastChild->m_nextSibling = animation;
    else
        m_firstChild = animation;
    animation->m_previousSibling = m_lastChild;
    m_lastChild = animation;

    animation->m_group = this;
    animationInserted(animation);
}

void QAnimationGroup2::prependAnimation(QAbstractAnimation2 *animation)
{
    if (QAnimationGroup2 *oldGroup = animation->m_group)
        oldGroup->removeAnimation(animation);

    Q_ASSERT(!previousSibling() && !nextSibling());

    if (m_firstChild)
        m_firstChild->m_previousSibling = animation;
    else
        m_lastChild = animation;
    animation->m_nextSibling = m_firstChild;
    m_firstChild = animation;

    animation->m_group = this;
    animationInserted(animation);
}

void QAnimationGroup2::removeAnimation(QAbstractAnimation2 *animation)
{
    Q_ASSERT(animation);
    Q_ASSERT(animation->m_group == this);
    QAbstractAnimation2 *prev = animation->previousSibling();
    QAbstractAnimation2 *next = animation->nextSibling();

    if (prev)
        prev->m_nextSibling = next;
    else
        m_firstChild = next;

    if (next)
        next->m_previousSibling = prev;
    else
        m_lastChild = prev;

    animation->m_previousSibling = 0;
    animation->m_nextSibling = 0;

    animation->m_group = 0;
    animationRemoved(animation, prev, next);
}

void QAnimationGroup2::clear()
{
    //### should this remove and delete, or just remove?
    while (firstChild() != 0)
        delete firstChild(); //removeAnimation(firstChild());
}

void QAnimationGroup2::resetUncontrolledAnimationsFinishTime()
{
    for (QAbstractAnimation2 *animation = firstChild(); animation; animation = animation->nextSibling()) {
        if (animation->duration() == -1 || animation->loopCount() < 0) {
            resetUncontrolledAnimationFinishTime(animation);
        }
    }
}

void QAnimationGroup2::resetUncontrolledAnimationFinishTime(QAbstractAnimation2 *anim)
{
    setUncontrolledAnimationFinishTime(anim, -1);
}

void QAnimationGroup2::setUncontrolledAnimationFinishTime(QAbstractAnimation2 *anim, int time)
{
    anim->m_uncontrolledFinishTime = time;
}

void QAnimationGroup2::uncontrolledAnimationFinished(QAbstractAnimation2 *animation)
{
    Q_UNUSED(animation);
}

void QAnimationGroup2::animationRemoved(QAbstractAnimation2* anim, QAbstractAnimation2* , QAbstractAnimation2* )
{
    Q_UNUSED(index);
    resetUncontrolledAnimationFinishTime(anim);
    if (!firstChild()) {
        m_currentTime = 0;
        stop();
    }
}

QT_END_NAMESPACE
