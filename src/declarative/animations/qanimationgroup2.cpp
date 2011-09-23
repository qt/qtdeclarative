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

/*!
    \class QAnimationGroup2
    \brief The QAnimationGroup2 class is an abstract base class for groups of animations.
    \since 4.6
    \ingroup animation

    An animation group is a container for animations (subclasses of
    QAbstractAnimation2). A group is usually responsible for managing
    the \l{QAbstractAnimation2::State}{state} of its animations, i.e.,
    it decides when to start, stop, resume, and pause them. Currently,
    Qt provides two such groups: QParallelAnimationGroup2 and
    QSequentialAnimationGroup2. Look up their class descriptions for
    details.

    Since QAnimationGroup2 inherits from QAbstractAnimation2, you can
    combine groups, and easily construct complex animation graphs.
    You can query QAbstractAnimation2 for the group it belongs to
    (using the \l{QAbstractAnimation2::}{group()} function).

    To start a top-level animation group, you simply use the
    \l{QAbstractAnimation2::}{start()} function from
    QAbstractAnimation2. By a top-level animation group, we think of a
    group that itself is not contained within another group. Starting
    sub groups directly is not supported, and may lead to unexpected
    behavior.

    \omit OK, we'll put in a snippet on this here \endomit

    QAnimationGroup2 provides methods for adding and retrieving
    animations. Besides that, you can remove animations by calling
    remove(), and clear the animation group by calling
    clear(). You may keep track of changes in the group's
    animations by listening to QEvent::ChildAdded and
    QEvent::ChildRemoved events.

    \omit OK, let's find a snippet here as well. \endomit

    QAnimationGroup2 takes ownership of the animations it manages, and
    ensures that they are deleted when the animation group is deleted.

    \sa QAbstractAnimation2, QVariantAnimation2, {The Animation Framework}
*/

#include "private/qanimationgroup2_p.h"
#include <QtCore/qdebug.h>
#include <QtCore/qcoreevent.h>
#include "private/qanimationgroup2_p_p.h"



QT_BEGIN_NAMESPACE


/*!
    Constructs a QAnimationGroup2.
    \a parent is passed to QObject's constructor.
*/
QAnimationGroup2::QAnimationGroup2(QObject *parent)
    : QAbstractAnimation2(*new QAnimationGroup2Private, parent)
{
}

/*!
    \internal
*/
QAnimationGroup2::QAnimationGroup2(QAnimationGroup2Private &dd, QObject *parent)
    : QAbstractAnimation2(dd, parent)
{
}

/*!
    Destroys the animation group. It will also destroy all its animations.
*/
QAnimationGroup2::~QAnimationGroup2()
{
}

/*!
    Returns a pointer to the animation at \a index in this group. This
    function is useful when you need access to a particular animation.  \a
    index is between 0 and animationCount() - 1.

    \sa animationCount(), indexOfAnimation()
*/
QAbstractAnimation2 *QAnimationGroup2::animationAt(int index) const
{
    Q_D(const QAnimationGroup2);

    if (index < 0 || index >= d->animations.size()) {
        qWarning("QAnimationGroup2::animationAt: index is out of bounds");
        return 0;
    }

    return d->animations.at(index);
}


/*!
    Returns the number of animations managed by this group.

    \sa indexOfAnimation(), addAnimation(), animationAt()
*/
int QAnimationGroup2::animationCount() const
{
    Q_D(const QAnimationGroup2);
    return d->animations.size();
}

/*!
    Returns the index of \a animation. The returned index can be passed
    to the other functions that take an index as an argument.

    \sa insertAnimation(), animationAt(), takeAnimation()
*/
int QAnimationGroup2::indexOfAnimation(QAbstractAnimation2 *animation) const
{
    Q_D(const QAnimationGroup2);
    return d->animations.indexOf(animation);
}

/*!
    Adds \a animation to this group. This will call insertAnimation with
    index equals to animationCount().

    \note The group takes ownership of the animation.

    \sa removeAnimation()
*/
void QAnimationGroup2::addAnimation(QAbstractAnimation2 *animation)
{
    Q_D(QAnimationGroup2);
    insertAnimation(d->animations.count(), animation);
}

/*!
    Inserts \a animation into this animation group at \a index.
    If \a index is 0 the animation is inserted at the beginning.
    If \a index is animationCount(), the animation is inserted at the end.

    \note The group takes ownership of the animation.

    \sa takeAnimation(), addAnimation(), indexOfAnimation(), removeAnimation()
*/
void QAnimationGroup2::insertAnimation(int index, QAbstractAnimation2 *animation)
{
    Q_D(QAnimationGroup2);

    if (index < 0 || index > d->animations.size()) {
        qWarning("QAnimationGroup2::insertAnimation: index is out of bounds");
        return;
    }

    if (QAnimationGroup2 *oldGroup = animation->group())
        oldGroup->removeAnimation(animation);

    d->animations.insert(index, animation);
    QAbstractAnimation2Private::get(animation)->group = this;
    // this will make sure that ChildAdded event is sent to 'this'
    animation->setParent(this);
    d->animationInsertedAt(index);
}

/*!
    Removes \a animation from this group. The ownership of \a animation is
    transferred to the caller.

    \sa takeAnimation(), insertAnimation(), addAnimation()
*/
void QAnimationGroup2::removeAnimation(QAbstractAnimation2 *animation)
{
    Q_D(QAnimationGroup2);

    if (!animation) {
        qWarning("QAnimationGroup2::remove: cannot remove null animation");
        return;
    }
    int index = d->animations.indexOf(animation);
    if (index == -1) {
        qWarning("QAnimationGroup2::remove: animation is not part of this group");
        return;
    }

    takeAnimation(index);
}

/*!
    Returns the animation at \a index and removes it from the animation group.

    \note The ownership of the animation is transferred to the caller.

    \sa removeAnimation(), addAnimation(), insertAnimation(), indexOfAnimation()
*/
QAbstractAnimation2 *QAnimationGroup2::takeAnimation(int index)
{
    Q_D(QAnimationGroup2);
    if (index < 0 || index >= d->animations.size()) {
        qWarning("QAnimationGroup2::takeAnimation: no animation at index %d", index);
        return 0;
    }
    QAbstractAnimation2 *animation = d->animations.at(index);
    QAbstractAnimation2Private::get(animation)->group = 0;
    // ### removing from list before doing setParent to avoid inifinite recursion
    // in ChildRemoved event
    d->animations.removeAt(index);
    animation->setParent(0);
    d->animationRemoved(index, animation);
    return animation;
}

/*!
    Removes and deletes all animations in this animation group, and resets the current
    time to 0.

    \sa addAnimation(), removeAnimation()
*/
void QAnimationGroup2::clear()
{
    Q_D(QAnimationGroup2);
    qDeleteAll(d->animations);
}

/*!
    \reimp
*/
bool QAnimationGroup2::event(QEvent *event)
{
    Q_D(QAnimationGroup2);
    if (event->type() == QEvent::ChildAdded) {
        QChildEvent *childEvent = static_cast<QChildEvent *>(event);
        if (QAbstractAnimation2 *a = qobject_cast<QAbstractAnimation2 *>(childEvent->child())) {
            if (a->group() != this)
                addAnimation(a);
        }
    } else if (event->type() == QEvent::ChildRemoved) {
        QChildEvent *childEvent = static_cast<QChildEvent *>(event);
        QAbstractAnimation2 *a = static_cast<QAbstractAnimation2 *>(childEvent->child());
        // You can only rely on the child being a QObject because in the QEvent::ChildRemoved
        // case it might be called from the destructor.
        int index = d->animations.indexOf(a);
        if (index != -1)
            takeAnimation(index);
    }
    return QAbstractAnimation2::event(event);
}


void QAnimationGroup2Private::animationRemoved(int index, QAbstractAnimation2 *)
{
    Q_Q(QAnimationGroup2);
    Q_UNUSED(index);
    if (animations.isEmpty()) {
        currentTime = 0;
        q->stop();
    }
}

QT_END_NAMESPACE

#include "moc_qanimationgroup2_p.cpp"


