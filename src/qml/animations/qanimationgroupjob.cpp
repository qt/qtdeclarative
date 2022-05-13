// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "private/qanimationgroupjob_p.h"

QT_BEGIN_NAMESPACE

QAnimationGroupJob::QAnimationGroupJob()
{
    m_isGroup = true;
}

void QAnimationGroupJob::ungroupChild(QAbstractAnimationJob *animation)
{
    Q_ASSERT(animation);
    Q_ASSERT(animation->m_group == this);
    m_children.remove(animation);
    animation->m_group = nullptr;
}

void QAnimationGroupJob::handleAnimationRemoved(QAbstractAnimationJob *animation)
{
    resetUncontrolledAnimationFinishTime(animation);
    if (m_children.isEmpty()) {
        m_currentTime = 0;
        stop();
    }
}

QAnimationGroupJob::~QAnimationGroupJob()
{
    while (QAbstractAnimationJob *animation = m_children.first()) {
        ungroupChild(animation);
        handleAnimationRemoved(animation);
        delete animation;
    }
}

void QAnimationGroupJob::topLevelAnimationLoopChanged()
{
    for (QAbstractAnimationJob *animation : m_children)
        animation->fireTopLevelAnimationLoopChanged();
}

void QAnimationGroupJob::appendAnimation(QAbstractAnimationJob *animation)
{
    if (QAnimationGroupJob *oldGroup = animation->m_group)
        oldGroup->removeAnimation(animation);

    Q_ASSERT(!animation->isInList());

    m_children.append(animation);
    animation->m_group = this;
    animationInserted(animation);
}

void QAnimationGroupJob::prependAnimation(QAbstractAnimationJob *animation)
{
    if (QAnimationGroupJob *oldGroup = animation->m_group)
        oldGroup->removeAnimation(animation);

    Q_ASSERT(!animation->isInList());

    m_children.prepend(animation);
    animation->m_group = this;
    animationInserted(animation);
}

void QAnimationGroupJob::removeAnimation(QAbstractAnimationJob *animation)
{
    QAbstractAnimationJob *prev = m_children.prev(animation);
    QAbstractAnimationJob *next = m_children.next(animation);
    ungroupChild(animation);
    animationRemoved(animation, prev, next);
}

void QAnimationGroupJob::clear()
{
    while (QAbstractAnimationJob *child = m_children.first()) {
        removeAnimation(child);
        delete child;
    }

    Q_ASSERT(m_children.isEmpty());
}

void QAnimationGroupJob::resetUncontrolledAnimationsFinishTime()
{
    for (QAbstractAnimationJob *animation : m_children) {
        if (animation->duration() == -1 || animation->loopCount() < 0) {
            resetUncontrolledAnimationFinishTime(animation);
        }
    }
}

void QAnimationGroupJob::resetUncontrolledAnimationFinishTime(QAbstractAnimationJob *anim)
{
    setUncontrolledAnimationFinishTime(anim, -1);
}

void QAnimationGroupJob::setUncontrolledAnimationFinishTime(QAbstractAnimationJob *anim, int time)
{
    anim->m_uncontrolledFinishTime = time;
}

void QAnimationGroupJob::uncontrolledAnimationFinished(QAbstractAnimationJob *animation)
{
    Q_UNUSED(animation);
}

void QAnimationGroupJob::animationRemoved(QAbstractAnimationJob* anim, QAbstractAnimationJob* , QAbstractAnimationJob* )
{
    handleAnimationRemoved(anim);
}

void QAnimationGroupJob::debugChildren(QDebug d) const
{
    int indentLevel = 1;
    const QAnimationGroupJob *group = this;
    while ((group = group->m_group))
        ++indentLevel;

    QByteArray ind(indentLevel, ' ');
    for (const QAbstractAnimationJob *child : m_children)
        d << "\n" << ind.constData() << child;
}

QT_END_NAMESPACE
