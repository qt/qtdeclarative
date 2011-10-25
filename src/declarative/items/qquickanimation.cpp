/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qquickanimation_p.h"
#include "qquickanimation_p_p.h"
#include "qquickstateoperations_p.h"

#include <private/qdeclarativeproperty_p.h>
#include <private/qdeclarativepath_p.h>

#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtCore/qmath.h>
#include "private/qsequentialanimationgroup2_p.h"
#include "private/qparallelanimationgroup2_p.h"
#include <QtGui/qtransform.h>

QT_BEGIN_NAMESPACE

QQuickParentAnimation::QQuickParentAnimation(QObject *parent)
    : QDeclarativeAnimationGroup(*(new QQuickParentAnimationPrivate), parent)
{
}

QQuickParentAnimation::~QQuickParentAnimation()
{
}

QQuickItem *QQuickParentAnimation::target() const
{
    Q_D(const QQuickParentAnimation);
    return d->target;
}

void QQuickParentAnimation::setTarget(QQuickItem *target)
{
    Q_D(QQuickParentAnimation);
    if (target == d->target)
        return;

    d->target = target;
    emit targetChanged();
}

QQuickItem *QQuickParentAnimation::newParent() const
{
    Q_D(const QQuickParentAnimation);
    return d->newParent;
}

void QQuickParentAnimation::setNewParent(QQuickItem *newParent)
{
    Q_D(QQuickParentAnimation);
    if (newParent == d->newParent)
        return;

    d->newParent = newParent;
    emit newParentChanged();
}

QQuickItem *QQuickParentAnimation::via() const
{
    Q_D(const QQuickParentAnimation);
    return d->via;
}

void QQuickParentAnimation::setVia(QQuickItem *via)
{
    Q_D(QQuickParentAnimation);
    if (via == d->via)
        return;

    d->via = via;
    emit viaChanged();
}

//### mirrors same-named function in QQuickItem
QPointF QQuickParentAnimationPrivate::computeTransformOrigin(QQuickItem::TransformOrigin origin, qreal width, qreal height) const
{
    switch (origin) {
    default:
    case QQuickItem::TopLeft:
        return QPointF(0, 0);
    case QQuickItem::Top:
        return QPointF(width / 2., 0);
    case QQuickItem::TopRight:
        return QPointF(width, 0);
    case QQuickItem::Left:
        return QPointF(0, height / 2.);
    case QQuickItem::Center:
        return QPointF(width / 2., height / 2.);
    case QQuickItem::Right:
        return QPointF(width, height / 2.);
    case QQuickItem::BottomLeft:
        return QPointF(0, height);
    case QQuickItem::Bottom:
        return QPointF(width / 2., height);
    case QQuickItem::BottomRight:
        return QPointF(width, height);
    }
}

QAbstractAnimation2Pointer QQuickParentAnimation::transition(QDeclarativeStateActions &actions,
                        QDeclarativeProperties &modified,
                        TransitionDirection direction)
{
    Q_D(QQuickParentAnimation);

    struct QQuickParentAnimationData : public QAbstractAnimationAction
    {
        QQuickParentAnimationData() {}
        ~QQuickParentAnimationData() { qDeleteAll(pc); }

        QDeclarativeStateActions actions;
        //### reverse should probably apply on a per-action basis
        bool reverse;
        QList<QQuickParentChange *> pc;
        virtual void doAction()
        {
            for (int ii = 0; ii < actions.count(); ++ii) {
                const QDeclarativeAction &action = actions.at(ii);
                if (reverse)
                    action.event->reverse();
                else
                    action.event->execute();
            }
        }
    };

    QQuickParentAnimationData *data = new QQuickParentAnimationData;
    QQuickParentAnimationData *viaData = new QQuickParentAnimationData;

    bool hasExplicit = false;
    if (d->target && d->newParent) {
        data->reverse = false;
        QDeclarativeAction myAction;
        QQuickParentChange *pc = new QQuickParentChange;
        pc->setObject(d->target);
        pc->setParent(d->newParent);
        myAction.event = pc;
        data->pc << pc;
        data->actions << myAction;
        hasExplicit = true;
        if (d->via) {
            viaData->reverse = false;
            QDeclarativeAction myVAction;
            QQuickParentChange *vpc = new QQuickParentChange;
            vpc->setObject(d->target);
            vpc->setParent(d->via);
            myVAction.event = vpc;
            viaData->pc << vpc;
            viaData->actions << myVAction;
        }
        //### once actions have concept of modified,
        //    loop to match appropriate ParentChanges and mark as modified
    }

    if (!hasExplicit)
    for (int i = 0; i < actions.size(); ++i) {
        QDeclarativeAction &action = actions[i];
        if (action.event && action.event->typeName() == QLatin1String("ParentChange")
            && (!d->target || static_cast<QQuickParentChange*>(action.event)->object() == d->target)) {

            QQuickParentChange *pc = static_cast<QQuickParentChange*>(action.event);
            QDeclarativeAction myAction = action;
            data->reverse = action.reverseEvent;

            //### this logic differs from PropertyAnimation
            //    (probably a result of modified vs. done)
            if (d->newParent) {
                QQuickParentChange *epc = new QQuickParentChange;
                epc->setObject(static_cast<QQuickParentChange*>(action.event)->object());
                epc->setParent(d->newParent);
                myAction.event = epc;
                data->pc << epc;
                data->actions << myAction;
                pc = epc;
            } else {
                action.actionDone = true;
                data->actions << myAction;
            }

            if (d->via) {
                viaData->reverse = false;
                QDeclarativeAction myAction;
                QQuickParentChange *vpc = new QQuickParentChange;
                vpc->setObject(pc->object());
                vpc->setParent(d->via);
                myAction.event = vpc;
                viaData->pc << vpc;
                viaData->actions << myAction;
                QDeclarativeAction dummyAction;
                QDeclarativeAction &xAction = pc->xIsSet() && i < actions.size()-1 ? actions[++i] : dummyAction;
                QDeclarativeAction &yAction = pc->yIsSet() && i < actions.size()-1 ? actions[++i] : dummyAction;
                QDeclarativeAction &sAction = pc->scaleIsSet() && i < actions.size()-1 ? actions[++i] : dummyAction;
                QDeclarativeAction &rAction = pc->rotationIsSet() && i < actions.size()-1 ? actions[++i] : dummyAction;
                QQuickItem *target = pc->object();
                QQuickItem *targetParent = action.reverseEvent ? pc->originalParent() : pc->parent();

                //### this mirrors the logic in QQuickParentChange.
                bool ok;
                const QTransform &transform = targetParent->itemTransform(d->via, &ok);
                if (transform.type() >= QTransform::TxShear || !ok) {
                    qmlInfo(this) << QQuickParentAnimation::tr("Unable to preserve appearance under complex transform");
                    ok = false;
                }

                qreal scale = 1;
                qreal rotation = 0;
                bool isRotate = (transform.type() == QTransform::TxRotate) || (transform.m11() < 0);
                if (ok && !isRotate) {
                    if (transform.m11() == transform.m22())
                        scale = transform.m11();
                    else {
                        qmlInfo(this) << QQuickParentAnimation::tr("Unable to preserve appearance under non-uniform scale");
                        ok = false;
                    }
                } else if (ok && isRotate) {
                    if (transform.m11() == transform.m22())
                        scale = qSqrt(transform.m11()*transform.m11() + transform.m12()*transform.m12());
                    else {
                        qmlInfo(this) << QQuickParentAnimation::tr("Unable to preserve appearance under non-uniform scale");
                        ok = false;
                    }

                    if (scale != 0)
                        rotation = atan2(transform.m12()/scale, transform.m11()/scale) * 180/M_PI;
                    else {
                        qmlInfo(this) << QQuickParentAnimation::tr("Unable to preserve appearance under scale of 0");
                        ok = false;
                    }
                }

                const QPointF &point = transform.map(QPointF(xAction.toValue.toReal(),yAction.toValue.toReal()));
                qreal x = point.x();
                qreal y = point.y();
                if (ok && target->transformOrigin() != QQuickItem::TopLeft) {
                    qreal w = target->width();
                    qreal h = target->height();
                    if (pc->widthIsSet() && i < actions.size() - 1)
                        w = actions[++i].toValue.toReal();
                    if (pc->heightIsSet() && i < actions.size() - 1)
                        h = actions[++i].toValue.toReal();
                    const QPointF &transformOrigin
                            = d->computeTransformOrigin(target->transformOrigin(), w,h);
                    qreal tempxt = transformOrigin.x();
                    qreal tempyt = transformOrigin.y();
                    QTransform t;
                    t.translate(-tempxt, -tempyt);
                    t.rotate(rotation);
                    t.scale(scale, scale);
                    t.translate(tempxt, tempyt);
                    const QPointF &offset = t.map(QPointF(0,0));
                    x += offset.x();
                    y += offset.y();
                }

                if (ok) {
                    //qDebug() << x << y << rotation << scale;
                    xAction.toValue = x;
                    yAction.toValue = y;
                    sAction.toValue = sAction.toValue.toReal() * scale;
                    rAction.toValue = rAction.toValue.toReal() + rotation;
                }
            }
        }
    }

    QSequentialAnimationGroup2 *topLevelGroup = new QSequentialAnimationGroup2;
    QActionAnimation *viaAction = d->via ? new QActionAnimation : 0;
    QActionAnimation *targetAction = new QActionAnimation;
    //we'll assume the common case by far is to have children, and always create ag
    QParallelAnimationGroup2 *ag = new QParallelAnimationGroup2;

    if (data->actions.count()) {
        if (d->via)
            viaAction->setAnimAction(viaData);
        targetAction->setAnimAction(data);

        //take care of any child animations
        bool valid = d->defaultProperty.isValid();
        QAbstractAnimation2Pointer anim;
        for (int ii = 0; ii < d->animations.count(); ++ii) {
            if (valid)
                d->animations.at(ii)->setDefaultTarget(d->defaultProperty);
            anim = d->animations.at(ii)->transition(actions, modified, direction);
            ag->addAnimation(anim);
        }

        //TODO: simplify/clarify logic
        bool forwards = direction == QDeclarativeAbstractAnimation::Forward;
        if (forwards) {
            topLevelGroup->addAnimation(d->via ? viaAction : targetAction);
            topLevelGroup->addAnimation(ag);
            if (d->via)
                topLevelGroup->addAnimation(targetAction);
        } else {
            if (d->via)
                topLevelGroup->addAnimation(targetAction);
            topLevelGroup->addAnimation(ag);
            topLevelGroup->addAnimation(d->via ? viaAction : targetAction);
        }
    } else {
        delete data;
        delete viaData;
    }

    return topLevelGroup;
}

QQuickAnchorAnimation::QQuickAnchorAnimation(QObject *parent)
: QDeclarativeAbstractAnimation(*(new QQuickAnchorAnimationPrivate), parent)
{
}

QQuickAnchorAnimation::~QQuickAnchorAnimation()
{
}

QDeclarativeListProperty<QQuickItem> QQuickAnchorAnimation::targets()
{
    Q_D(QQuickAnchorAnimation);
    return QDeclarativeListProperty<QQuickItem>(this, d->targets);
}

int QQuickAnchorAnimation::duration() const
{
    Q_D(const QQuickAnchorAnimation);
    return d->duration;
}

void QQuickAnchorAnimation::setDuration(int duration)
{
    if (duration < 0) {
        qmlInfo(this) << tr("Cannot set a duration of < 0");
        return;
    }

    Q_D(QQuickAnchorAnimation);
    if (d->duration == duration)
        return;
    d->duration = duration;
    emit durationChanged(duration);
}

QEasingCurve QQuickAnchorAnimation::easing() const
{
    Q_D(const QQuickAnchorAnimation);
    return d->easing;
}

void QQuickAnchorAnimation::setEasing(const QEasingCurve &e)
{
    Q_D(QQuickAnchorAnimation);
    if (d->easing == e)
        return;

    d->easing = e;
    emit easingChanged(e);
}

QAbstractAnimation2Pointer QQuickAnchorAnimation::transition(QDeclarativeStateActions &actions,
                        QDeclarativeProperties &modified,
                        TransitionDirection direction)
{
    Q_UNUSED(modified);
    Q_D(QQuickAnchorAnimation);
    QDeclarativeAnimationPropertyUpdater *data = new QDeclarativeAnimationPropertyUpdater;
    data->interpolatorType = QMetaType::QReal;
    data->interpolator = d->interpolator;
    data->reverse = direction == Backward ? true : false;
    data->fromSourced = false;
    data->fromDefined = false;

    for (int ii = 0; ii < actions.count(); ++ii) {
        QDeclarativeAction &action = actions[ii];
        if (action.event && action.event->typeName() == QLatin1String("AnchorChanges")
            && (d->targets.isEmpty() || d->targets.contains(static_cast<QQuickAnchorChanges*>(action.event)->object()))) {
            data->actions << static_cast<QQuickAnchorChanges*>(action.event)->additionalActions();
        }
    }

    QDeclarativeBulkValueAnimator *animator = new QDeclarativeBulkValueAnimator;
    if (data->actions.count()) {
        animator->setAnimValue(data);
        animator->setFromSourcedValue(&data->fromSourced);
    } else {
        delete data;
    }
    return animator;
}

QQuickPathAnimation::QQuickPathAnimation(QObject *parent)
: QDeclarativeAbstractAnimation(*(new QQuickPathAnimationPrivate), parent)
{
    Q_D(QQuickPathAnimation);
    d->pa = new QDeclarativeBulkValueAnimator(this);
}

QQuickPathAnimation::~QQuickPathAnimation()
{
}

int QQuickPathAnimation::duration() const
{
    Q_D(const QQuickPathAnimation);
    return d->pa->duration();
}

void QQuickPathAnimation::setDuration(int duration)
{
    if (duration < 0) {
        qmlInfo(this) << tr("Cannot set a duration of < 0");
        return;
    }

    Q_D(QQuickPathAnimation);
    if (d->pa->duration() == duration)
        return;
    d->pa->setDuration(duration);
    emit durationChanged(duration);
}

QEasingCurve QQuickPathAnimation::easing() const
{
    Q_D(const QQuickPathAnimation);
    return d->pa->easingCurve();
}

void QQuickPathAnimation::setEasing(const QEasingCurve &e)
{
    Q_D(QQuickPathAnimation);
    if (d->pa->easingCurve() == e)
        return;

    d->pa->setEasingCurve(e);
    emit easingChanged(e);
}

QDeclarativePath *QQuickPathAnimation::path() const
{
    Q_D(const QQuickPathAnimation);
    return d->path;
}

void QQuickPathAnimation::setPath(QDeclarativePath *path)
{
    Q_D(QQuickPathAnimation);
    if (d->path == path)
        return;

    d->path = path;
    emit pathChanged();
}

QQuickItem *QQuickPathAnimation::target() const
{
    Q_D(const QQuickPathAnimation);
    return d->target;
}

void QQuickPathAnimation::setTarget(QQuickItem *target)
{
    Q_D(QQuickPathAnimation);
    if (d->target == target)
        return;

    d->target = target;
    emit targetChanged();
}

QQuickPathAnimation::Orientation QQuickPathAnimation::orientation() const
{
    Q_D(const QQuickPathAnimation);
    return d->orientation;
}

void QQuickPathAnimation::setOrientation(Orientation orientation)
{
    Q_D(QQuickPathAnimation);
    if (d->orientation == orientation)
        return;

    d->orientation = orientation;
    emit orientationChanged(d->orientation);
}

QPointF QQuickPathAnimation::anchorPoint() const
{
    Q_D(const QQuickPathAnimation);
    return d->anchorPoint;
}

void QQuickPathAnimation::setAnchorPoint(const QPointF &point)
{
    Q_D(QQuickPathAnimation);
    if (d->anchorPoint == point)
        return;

    d->anchorPoint = point;
    emit anchorPointChanged(point);
}

qreal QQuickPathAnimation::orientationEntryInterval() const
{
    Q_D(const QQuickPathAnimation);
    return d->entryInterval;
}

void QQuickPathAnimation::setOrientationEntryInterval(qreal interval)
{
    Q_D(QQuickPathAnimation);
    if (d->entryInterval == interval)
        return;
    d->entryInterval = interval;
    emit orientationEntryIntervalChanged(interval);
}

qreal QQuickPathAnimation::orientationExitInterval() const
{
    Q_D(const QQuickPathAnimation);
    return d->exitInterval;
}

void QQuickPathAnimation::setOrientationExitInterval(qreal interval)
{
    Q_D(QQuickPathAnimation);
    if (d->exitInterval == interval)
        return;
    d->exitInterval = interval;
    emit orientationExitIntervalChanged(interval);
}

qreal QQuickPathAnimation::endRotation() const
{
    Q_D(const QQuickPathAnimation);
    return d->endRotation.isNull ? qreal(0) : d->endRotation.value;
}

void QQuickPathAnimation::setEndRotation(qreal rotation)
{
    Q_D(QQuickPathAnimation);
    if (!d->endRotation.isNull && d->endRotation == rotation)
        return;

    d->endRotation = rotation;
    emit endRotationChanged(d->endRotation);
}

QAbstractAnimation2Pointer QQuickPathAnimation::transition(QDeclarativeStateActions &actions,
                                           QDeclarativeProperties &modified,
                                           TransitionDirection direction)
{
    Q_D(QQuickPathAnimation);
    QQuickPathAnimationUpdater *data = new QQuickPathAnimationUpdater;

    data->orientation = d->orientation;
    data->anchorPoint = d->anchorPoint;
    data->entryInterval = d->entryInterval;
    data->exitInterval = d->exitInterval;
    data->endRotation = d->endRotation;
    data->reverse = direction == Backward ? true : false;
    data->fromSourced = false;
    data->fromDefined = (d->path && d->path->hasStartX() && d->path->hasStartY()) ? true : false;
    data->toDefined = d->path ? d->path->hasEnd() : false;
    int origModifiedSize = modified.count();

    for (int i = 0; i < actions.count(); ++i) {
        QDeclarativeAction &action = actions[i];
        if (action.event)
            continue;
        if (action.specifiedObject == d->target && action.property.name() == QLatin1String("x")) {
            data->toX = action.toValue.toReal();
            modified << action.property;
            action.fromValue = action.toValue;
        }
        if (action.specifiedObject == d->target && action.property.name() == QLatin1String("y")) {
            data->toY = action.toValue.toReal();
            modified << action.property;
            action.fromValue = action.toValue;
        }
    }

    if (d->target && d->path &&
        (modified.count() > origModifiedSize || data->toDefined)) {
        data->target = d->target;
        data->path = d->path;
        /*
            NOTE: The following block relies on the fact that the previous value hasn't
            yet been deleted, and has the same target, etc, which may be a bit fragile.
         */
        if (d->pa->getAnimValue()) {
            QQuickPathAnimationUpdater *prevData = static_cast<QQuickPathAnimationUpdater*>(d->pa->getAnimValue());

            // get the original start angle that was used (so we can exactly reverse).
            data->startRotation = prevData->startRotation;

            // treat interruptions specially, otherwise we end up with strange paths
            if ((data->reverse || prevData->reverse) && prevData->currentV > 0 && prevData->currentV < 1) {
                if (!data->fromDefined && !data->toDefined && !prevData->painterPath.isEmpty()) {
                    QPointF pathPos = QDeclarativePath::sequentialPointAt(prevData->painterPath, prevData->pathLength, prevData->attributePoints, prevData->prevBez, prevData->currentV);
                    if (!prevData->anchorPoint.isNull())
                        pathPos -= prevData->anchorPoint;
                    if (pathPos == data->target->pos()) {   //only treat as interruption if we interrupted ourself
                        data->painterPath = prevData->painterPath;
                        data->toDefined = data->fromDefined = data->fromSourced = true;
                        data->prevBez.isValid = false;
                        data->interruptStart = prevData->currentV;
                        data->startRotation = prevData->startRotation;
                        data->pathLength = prevData->pathLength;
                        data->attributePoints = prevData->attributePoints;
                    }
                }
            }
        }
        d->pa->setFromSourcedValue(&data->fromSourced);
        d->pa->setAnimValue(data);
    } else {
        d->pa->setFromSourcedValue(0);
        d->pa->setAnimValue(0);
        delete data;
    }
    return d->pa;
}

void QQuickPathAnimationUpdater::setValue(qreal v)
{
    if (interruptStart.isValid()) {
        if (reverse)
            v = 1 - v;
        qreal end = reverse ? 0.0 : 1.0;
        v = interruptStart + v * (end-interruptStart);
    }
    currentV = v;
    bool atStart = ((reverse && v == 1.0) || (!reverse && v == 0.0));
    if (!fromSourced && (!fromDefined || !toDefined)) {
        qreal startX = reverse ? toX + anchorPoint.x() : target->x() + anchorPoint.x();
        qreal startY = reverse ? toY + anchorPoint.y() : target->y() + anchorPoint.y();
        qreal endX = reverse ? target->x() + anchorPoint.x() : toX + anchorPoint.x();
        qreal endY = reverse ? target->y() + anchorPoint.y() : toY + anchorPoint.y();

        prevBez.isValid = false;
        painterPath = path->createPath(QPointF(startX, startY), QPointF(endX, endY), QStringList(), pathLength, attributePoints);
        fromSourced = true;
    }

    qreal angle;
    bool fixed = orientation == QQuickPathAnimation::Fixed;
    QPointF currentPos = !painterPath.isEmpty() ? path->sequentialPointAt(painterPath, pathLength, attributePoints, prevBez, v, fixed ? 0 : &angle) : path->sequentialPointAt(v, fixed ? 0 : &angle);

    //adjust position according to anchor point
    if (!anchorPoint.isNull()) {
        currentPos -= anchorPoint;
        if (atStart) {
            if (!anchorPoint.isNull() && !fixed)
                target->setTransformOriginPoint(anchorPoint);
        }
    }

    //### could cache properties rather than reconstructing each time
    QDeclarativePropertyPrivate::write(QDeclarativeProperty(target, QStringLiteral("x")), currentPos.x(), QDeclarativePropertyPrivate::BypassInterceptor | QDeclarativePropertyPrivate::DontRemoveBinding);
    QDeclarativePropertyPrivate::write(QDeclarativeProperty(target, QStringLiteral("y")), currentPos.y(), QDeclarativePropertyPrivate::BypassInterceptor | QDeclarativePropertyPrivate::DontRemoveBinding);

    //adjust angle according to orientation
    if (!fixed) {
        switch (orientation) {
            case QQuickPathAnimation::RightFirst:
                angle = -angle;
                break;
            case QQuickPathAnimation::TopFirst:
                angle = -angle + 90;
                break;
            case QQuickPathAnimation::LeftFirst:
                angle = -angle + 180;
                break;
            case QQuickPathAnimation::BottomFirst:
                angle = -angle + 270;
                break;
            default:
                angle = 0;
                break;
        }

        if (atStart && !reverse) {
            startRotation = target->rotation();

            //shortest distance to correct orientation
            qreal diff = angle - startRotation;
            while (diff > 180.0) {
                startRotation.value += 360.0;
                diff -= 360.0;
            }
            while (diff < -180.0) {
                startRotation.value -= 360.0;
                diff += 360.0;
            }
        }

        //smoothly transition to the desired orientation
        if (startRotation.isValid()) {
            if (reverse && v == 0.0)
                angle = startRotation;
            else if (v < entryInterval)
                angle = angle * v / entryInterval + startRotation * (entryInterval - v) / entryInterval;
        }
        if (endRotation.isValid()) {
            qreal exitStart = 1 - exitInterval;
            if (!reverse && v == 1.0)
                angle = endRotation;
            else if (v > exitStart)
                angle = endRotation * (v - exitStart) / exitInterval + angle * (exitInterval - (v - exitStart)) / exitInterval;
        }
        QDeclarativePropertyPrivate::write(QDeclarativeProperty(target, QStringLiteral("rotation")), angle, QDeclarativePropertyPrivate::BypassInterceptor | QDeclarativePropertyPrivate::DontRemoveBinding);
    }

    /*
        NOTE: we don't always reset the transform origin, as it can cause a
        visual jump if ending on an angle. This means that in some cases
        (anchor point and orientation both specified, and ending at an angle)
        the transform origin will always be set after running the path animation.
     */
    if ((reverse && v == 0.0) || (!reverse && v == 1.0)) {
        if (!anchorPoint.isNull() && !fixed && qFuzzyIsNull(angle))
            target->setTransformOriginPoint(QPointF());
    }
}

QT_END_NAMESPACE
