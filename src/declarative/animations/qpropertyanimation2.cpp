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
    \class QPropertyAnimation2
    \brief The QPropertyAnimation2 class animates Qt properties
    \since 4.6

    \ingroup animation

    QPropertyAnimation2 interpolates over \l{Qt's Property System}{Qt
    properties}. As property values are stored in \l{QVariant}s, the
    class inherits QVariantAnimation2, and supports animation of the
    same \l{QVariant::Type}{variant types} as its super class.

    A class declaring properties must be a QObject. To make it
    possible to animate a property, it must provide a setter (so that
    QPropertyAnimation2 can set the property's value). Note that this
    makes it possible to animate many of Qt's widgets. Let's look at
    an example:

    \code
        QPropertyAnimation2 *animation = new QPropertyAnimation2(myWidget, "geometry");
        animation->setDuration(10000);
        animation->setStartValue(QRect(0, 0, 100, 30));
        animation->setEndValue(QRect(250, 250, 100, 30));

        animation->start();
    \endcode

    The property name and the QObject instance of which property
    should be animated are passed to the constructor. You can then
    specify the start and end value of the property. The procedure is
    equal for properties in classes you have implemented
    yourself--just check with QVariantAnimation2 that your QVariant
    type is supported.

    The QVariantAnimation2 class description explains how to set up the
    animation in detail. Note, however, that if a start value is not
    set, the property will start at the value it had when the
    QPropertyAnimation2 instance was created.

    QPropertyAnimation2 works like a charm on its own. For complex
    animations that, for instance, contain several objects,
    QAnimationGroup2 is provided. An animation group is an animation
    that can contain other animations, and that can manage when its
    animations are played. Look at QParallelAnimationGroup2 for an
    example.

    \sa QVariantAnimation2, QAnimationGroup2, {The Animation Framework}
*/

#include "private/qpropertyanimation2_p.h"
#include "private/qanimationgroup2_p.h"
#include "private/qpropertyanimation2_p_p.h"

#include <private/qmutexpool_p.h>



QT_BEGIN_NAMESPACE

void QPropertyAnimation2Private::updateMetaProperty()
{
    if (!target || propertyName.isEmpty()) {
        propertyType = QVariant::Invalid;
        propertyIndex = -1;
        return;
    }

    //propertyType will be set to a valid type only if there is a Q_PROPERTY
    //otherwise it will be set to QVariant::Invalid at the end of this function
    propertyType = targetValue->property(propertyName).userType();
    propertyIndex = targetValue->metaObject()->indexOfProperty(propertyName);

    if (propertyType != QVariant::Invalid)
        convertValues(propertyType);
    if (propertyIndex == -1) {
        //there is no Q_PROPERTY on the object
        propertyType = QVariant::Invalid;
        if (!targetValue->dynamicPropertyNames().contains(propertyName))
            qWarning("QPropertyAnimation2: you're trying to animate a non-existing property %s of your QObject", propertyName.constData());
    } else if (!targetValue->metaObject()->property(propertyIndex).isWritable()) {
        qWarning("QPropertyAnimation2: you're trying to animate the non-writable property %s of your QObject", propertyName.constData());
	}
}

void QPropertyAnimation2Private::updateProperty(const QVariant &newValue)
{
    if (state == QAbstractAnimation2::Stopped)
        return;

    if (!target) {
        q_func()->stop(); //the target was destroyed we need to stop the animation
        return;
    }

    if (newValue.userType() == propertyType) {
        //no conversion is needed, we directly call the QMetaObject::metacall
        void *data = const_cast<void*>(newValue.constData());
        QMetaObject::metacall(targetValue, QMetaObject::WriteProperty, propertyIndex, &data);
    } else {
        targetValue->setProperty(propertyName.constData(), newValue);
    }
}

/*!
    Construct a QPropertyAnimation2 object. \a parent is passed to QObject's
    constructor.
*/
QPropertyAnimation2::QPropertyAnimation2(QObject *parent)
    : QVariantAnimation2(*new QPropertyAnimation2Private, parent)
{
}

/*!
    Construct a QPropertyAnimation2 object. \a parent is passed to QObject's
    constructor. The animation changes the property \a propertyName on \a
    target. The default duration is 250ms.

    \sa targetObject, propertyName
*/
QPropertyAnimation2::QPropertyAnimation2(QObject *target, const QByteArray &propertyName, QObject *parent)
    : QVariantAnimation2(*new QPropertyAnimation2Private, parent)
{
    setTargetObject(target);
    setPropertyName(propertyName);
}

/*!
    Destroys the QPropertyAnimation2 instance.
 */
QPropertyAnimation2::~QPropertyAnimation2()
{
    stop();
}

/*!
    \property QPropertyAnimation2::targetObject
    \brief the target QObject for this animation.

    This property defines the target QObject for this animation.
 */
QObject *QPropertyAnimation2::targetObject() const
{
    return d_func()->target.data();
}

void QPropertyAnimation2::setTargetObject(QObject *target)
{
    Q_D(QPropertyAnimation2);
    if (d->targetValue == target)
        return;

    if (d->state != QAbstractAnimation2::Stopped) {
        qWarning("QPropertyAnimation2::setTargetObject: you can't change the target of a running animation");
        return;
    }

    d->target = d->targetValue = target;
    d->updateMetaProperty();
}

/*!
    \property QPropertyAnimation2::propertyName
    \brief the target property name for this animation

    This property defines the target property name for this animation. The
    property name is required for the animation to operate.
 */
QByteArray QPropertyAnimation2::propertyName() const
{
    Q_D(const QPropertyAnimation2);
    return d->propertyName;
}

void QPropertyAnimation2::setPropertyName(const QByteArray &propertyName)
{
    Q_D(QPropertyAnimation2);
    if (d->state != QAbstractAnimation2::Stopped) {
        qWarning("QPropertyAnimation2::setPropertyName: you can't change the property name of a running animation");
        return;
    }

    d->propertyName = propertyName;
    d->updateMetaProperty();
}


/*!
    \reimp
 */
bool QPropertyAnimation2::event(QEvent *event)
{
    return QVariantAnimation2::event(event);
}

/*!
    This virtual function is called by QVariantAnimation2 whenever the current value
    changes. \a value is the new, updated value. It updates the current value
    of the property on the target object.

    \sa currentValue, currentTime
 */
void QPropertyAnimation2::updateCurrentValue(const QVariant &value)
{
    Q_D(QPropertyAnimation2);
    d->updateProperty(value);
}

/*!
    \reimp

    If the startValue is not defined when the state of the animation changes from Stopped to Running,
    the current property value is used as the initial value for the animation.
*/
void QPropertyAnimation2::updateState(QAbstractAnimation2::State newState,
                                     QAbstractAnimation2::State oldState)
{
    Q_D(QPropertyAnimation2);

    if (!d->target && oldState == Stopped) {
        qWarning("QPropertyAnimation2::updateState (%s): Changing state of an animation without target",
                 d->propertyName.constData());
        return;
    }

    QVariantAnimation2::updateState(newState, oldState);

    QPropertyAnimation2 *animToStop = 0;
    {
#ifndef QT_NO_THREAD
        QMutexLocker locker(QMutexPool::globalInstanceGet(&staticMetaObject));
#endif
        typedef QPair<QObject *, QByteArray> QPropertyAnimation2Pair;
        typedef QHash<QPropertyAnimation2Pair, QPropertyAnimation2*> QPropertyAnimation2Hash;
        static QPropertyAnimation2Hash hash;
        //here we need to use value because we need to know to which pointer
        //the animation was referring in case stopped because the target was destroyed
        QPropertyAnimation2Pair key(d->targetValue, d->propertyName);
        if (newState == Running) {
            d->updateMetaProperty();
            animToStop = hash.value(key, 0);
            hash.insert(key, this);
            // update the default start value
            if (oldState == Stopped) {
                d->setDefaultStartEndValue(d->targetValue->property(d->propertyName.constData()));
                //let's check if we have a start value and an end value
                if (!startValue().isValid() && (d->direction == Backward || !d->defaultStartEndValue.isValid())) {
                    qWarning("QPropertyAnimation2::updateState (%s, %s, %s): starting an animation without start value",
                             d->propertyName.constData(), d->target.data()->metaObject()->className(),
                             qPrintable(d->target.data()->objectName()));
                }
                if (!endValue().isValid() && (d->direction == Forward || !d->defaultStartEndValue.isValid())) {
                    qWarning("QPropertyAnimation2::updateState (%s, %s, %s): starting an animation without end value",
                             d->propertyName.constData(), d->target.data()->metaObject()->className(),
                             qPrintable(d->target.data()->objectName()));
                }
            }
        } else if (hash.value(key) == this) {
            hash.remove(key);
        }
    }

    //we need to do that after the mutex was unlocked
    if (animToStop) {
        // try to stop the top level group
        QAbstractAnimation2 *current = animToStop;
        while (current->group() && current->state() != Stopped)
            current = current->group();
        current->stop();
    }
}

#include "moc_qpropertyanimation2_p.cpp"

QT_END_NAMESPACE


