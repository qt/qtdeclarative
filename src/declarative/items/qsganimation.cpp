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

#include "qsganimation_p.h"
#include "qsganimation_p_p.h"
#include "qsgstateoperations_p.h"

#include <qdeclarativeproperty_p.h>
#include <qdeclarativepath_p.h>

#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtCore/qmath.h>
#include "private/qsequentialanimationgroup2_p.h"
#include "private/qparallelanimationgroup2_p.h"
#include <QtGui/qtransform.h>

QT_BEGIN_NAMESPACE

QSGParentAnimation::QSGParentAnimation(QObject *parent)
    : QDeclarativeAnimationGroup(*(new QSGParentAnimationPrivate), parent)
{
}

QSGParentAnimation::~QSGParentAnimation()
{
}

QSGItem *QSGParentAnimation::target() const
{
    Q_D(const QSGParentAnimation);
    return d->target;
}

void QSGParentAnimation::setTarget(QSGItem *target)
{
    Q_D(QSGParentAnimation);
    if (target == d->target)
        return;

    d->target = target;
    emit targetChanged();
}

QSGItem *QSGParentAnimation::newParent() const
{
    Q_D(const QSGParentAnimation);
    return d->newParent;
}

void QSGParentAnimation::setNewParent(QSGItem *newParent)
{
    Q_D(QSGParentAnimation);
    if (newParent == d->newParent)
        return;

    d->newParent = newParent;
    emit newParentChanged();
}

QSGItem *QSGParentAnimation::via() const
{
    Q_D(const QSGParentAnimation);
    return d->via;
}

void QSGParentAnimation::setVia(QSGItem *via)
{
    Q_D(QSGParentAnimation);
    if (via == d->via)
        return;

    d->via = via;
    emit viaChanged();
}

//### mirrors same-named function in QSGItem
QPointF QSGParentAnimationPrivate::computeTransformOrigin(QSGItem::TransformOrigin origin, qreal width, qreal height) const
{
    switch(origin) {
    default:
    case QSGItem::TopLeft:
        return QPointF(0, 0);
    case QSGItem::Top:
        return QPointF(width / 2., 0);
    case QSGItem::TopRight:
        return QPointF(width, 0);
    case QSGItem::Left:
        return QPointF(0, height / 2.);
    case QSGItem::Center:
        return QPointF(width / 2., height / 2.);
    case QSGItem::Right:
        return QPointF(width, height / 2.);
    case QSGItem::BottomLeft:
        return QPointF(0, height);
    case QSGItem::Bottom:
        return QPointF(width / 2., height);
    case QSGItem::BottomRight:
        return QPointF(width, height);
    }
}

QAbstractAnimation2Pointer QSGParentAnimation::transition(QDeclarativeStateActions &actions,
                        QDeclarativeProperties &modified,
                        TransitionDirection direction)
{
    Q_D(QSGParentAnimation);

    struct QSGParentAnimationData : public QAbstractAnimationAction
    {
        QSGParentAnimationData() {}
        ~QSGParentAnimationData() { qDeleteAll(pc); }

        QDeclarativeStateActions actions;
        //### reverse should probably apply on a per-action basis
        bool reverse;
        QList<QSGParentChange *> pc;
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

    QSGParentAnimationData *data = new QSGParentAnimationData;
    QSGParentAnimationData *viaData = new QSGParentAnimationData;

    bool hasExplicit = false;
    if (d->target && d->newParent) {
        data->reverse = false;
        QDeclarativeAction myAction;
        QSGParentChange *pc = new QSGParentChange;
        pc->setObject(d->target);
        pc->setParent(d->newParent);
        myAction.event = pc;
        data->pc << pc;
        data->actions << myAction;
        hasExplicit = true;
        if (d->via) {
            viaData->reverse = false;
            QDeclarativeAction myVAction;
            QSGParentChange *vpc = new QSGParentChange;
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
            && (!d->target || static_cast<QSGParentChange*>(action.event)->object() == d->target)) {

            QSGParentChange *pc = static_cast<QSGParentChange*>(action.event);
            QDeclarativeAction myAction = action;
            data->reverse = action.reverseEvent;

            //### this logic differs from PropertyAnimation
            //    (probably a result of modified vs. done)
            if (d->newParent) {
                QSGParentChange *epc = new QSGParentChange;
                epc->setObject(static_cast<QSGParentChange*>(action.event)->object());
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
                QSGParentChange *vpc = new QSGParentChange;
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
                QSGItem *target = pc->object();
                QSGItem *targetParent = action.reverseEvent ? pc->originalParent() : pc->parent();

                //### this mirrors the logic in QSGParentChange.
                bool ok;
                const QTransform &transform = targetParent->itemTransform(d->via, &ok);
                if (transform.type() >= QTransform::TxShear || !ok) {
                    qmlInfo(this) << QSGParentAnimation::tr("Unable to preserve appearance under complex transform");
                    ok = false;
                }

                qreal scale = 1;
                qreal rotation = 0;
                bool isRotate = (transform.type() == QTransform::TxRotate) || (transform.m11() < 0);
                if (ok && !isRotate) {
                    if (transform.m11() == transform.m22())
                        scale = transform.m11();
                    else {
                        qmlInfo(this) << QSGParentAnimation::tr("Unable to preserve appearance under non-uniform scale");
                        ok = false;
                    }
                } else if (ok && isRotate) {
                    if (transform.m11() == transform.m22())
                        scale = qSqrt(transform.m11()*transform.m11() + transform.m12()*transform.m12());
                    else {
                        qmlInfo(this) << QSGParentAnimation::tr("Unable to preserve appearance under non-uniform scale");
                        ok = false;
                    }

                    if (scale != 0)
                        rotation = atan2(transform.m12()/scale, transform.m11()/scale) * 180/M_PI;
                    else {
                        qmlInfo(this) << QSGParentAnimation::tr("Unable to preserve appearance under scale of 0");
                        ok = false;
                    }
                }

                const QPointF &point = transform.map(QPointF(xAction.toValue.toReal(),yAction.toValue.toReal()));
                qreal x = point.x();
                qreal y = point.y();
                if (ok && target->transformOrigin() != QSGItem::TopLeft) {
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

    //TODO: only create necessary animations
    QSequentialAnimationGroup2 *topLevelGroup = new QSequentialAnimationGroup2;
    QActionAnimation *startAction = new QActionAnimation;
    QActionAnimation *endAction = new QActionAnimation;
    QParallelAnimationGroup2 *ag = new QParallelAnimationGroup2;

    if (data->actions.count()) {
        if (direction == QDeclarativeAbstractAnimation::Forward) {
            startAction->setAnimAction(d->via ? viaData : data);
            endAction->setAnimAction(d->via ? data : 0);
        } else {
            endAction->setAnimAction(d->via ? viaData : data);
            startAction->setAnimAction(d->via ? data : 0);
        }
    } else {
        delete data;
        delete viaData;
    }

    //take care of any child animations
    bool valid = d->defaultProperty.isValid();
    QAbstractAnimation2Pointer anim;
    for (int ii = 0; ii < d->animations.count(); ++ii) {
        if (valid)
            d->animations.at(ii)->setDefaultTarget(d->defaultProperty);
        anim = d->animations.at(ii)->transition(actions, modified, direction);
        ag->addAnimation(anim);
    }

    topLevelGroup->addAnimation(startAction);
    topLevelGroup->addAnimation(ag);
    topLevelGroup->addAnimation(endAction);

    return topLevelGroup;
}

QSGAnchorAnimation::QSGAnchorAnimation(QObject *parent)
: QDeclarativeAbstractAnimation(*(new QSGAnchorAnimationPrivate), parent)
{
}

QSGAnchorAnimation::~QSGAnchorAnimation()
{
}

QDeclarativeListProperty<QSGItem> QSGAnchorAnimation::targets()
{
    Q_D(QSGAnchorAnimation);
    return QDeclarativeListProperty<QSGItem>(this, d->targets);
}

int QSGAnchorAnimation::duration() const
{
    Q_D(const QSGAnchorAnimation);
    return d->duration;
}

void QSGAnchorAnimation::setDuration(int duration)
{
    if (duration < 0) {
        qmlInfo(this) << tr("Cannot set a duration of < 0");
        return;
    }

    Q_D(QSGAnchorAnimation);
    if (d->duration == duration)
        return;
    d->duration = duration;
    emit durationChanged(duration);
}

QEasingCurve QSGAnchorAnimation::easing() const
{
    Q_D(const QSGAnchorAnimation);
    return d->easing;
}

void QSGAnchorAnimation::setEasing(const QEasingCurve &e)
{
    Q_D(QSGAnchorAnimation);
    if (d->easing == e)
        return;

    d->easing = e;
    emit easingChanged(e);
}

QAbstractAnimation2Pointer QSGAnchorAnimation::transition(QDeclarativeStateActions &actions,
                        QDeclarativeProperties &modified,
                        TransitionDirection direction)
{
    Q_UNUSED(modified);
    Q_D(QSGAnchorAnimation);
    QDeclarativeAnimationPropertyUpdater *data = new QDeclarativeAnimationPropertyUpdater;
    data->interpolatorType = QMetaType::QReal;
    data->interpolator = d->interpolator;
    data->reverse = direction == Backward ? true : false;
    data->fromSourced = false;
    data->fromDefined = false;

    for (int ii = 0; ii < actions.count(); ++ii) {
        QDeclarativeAction &action = actions[ii];
        if (action.event && action.event->typeName() == QLatin1String("AnchorChanges")
            && (d->targets.isEmpty() || d->targets.contains(static_cast<QSGAnchorChanges*>(action.event)->object()))) {
            data->actions << static_cast<QSGAnchorChanges*>(action.event)->additionalActions();
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

QSGPathAnimation::QSGPathAnimation(QObject *parent)
: QDeclarativeAbstractAnimation(*(new QSGPathAnimationPrivate), parent)
{
    Q_D(QSGPathAnimation);
    d->pa = new QDeclarativeBulkValueAnimator(this);
}

QSGPathAnimation::~QSGPathAnimation()
{
}

int QSGPathAnimation::duration() const
{
    Q_D(const QSGPathAnimation);
    return d->pa->duration();
}

void QSGPathAnimation::setDuration(int duration)
{
    if (duration < 0) {
        qmlInfo(this) << tr("Cannot set a duration of < 0");
        return;
    }

    Q_D(QSGPathAnimation);
    if (d->pa->duration() == duration)
        return;
    d->pa->setDuration(duration);
    emit durationChanged(duration);
}

QEasingCurve QSGPathAnimation::easing() const
{
    Q_D(const QSGPathAnimation);
    return d->pa->easingCurve();
}

void QSGPathAnimation::setEasing(const QEasingCurve &e)
{
    Q_D(QSGPathAnimation);
    if (d->pa->easingCurve() == e)
        return;

    d->pa->setEasingCurve(e);
    emit easingChanged(e);
}

QDeclarativePath *QSGPathAnimation::path() const
{
    Q_D(const QSGPathAnimation);
    return d->path;
}

void QSGPathAnimation::setPath(QDeclarativePath *path)
{
    Q_D(QSGPathAnimation);
    if (d->path == path)
        return;

    d->path = path;
    emit pathChanged();
}

QSGItem *QSGPathAnimation::target() const
{
    Q_D(const QSGPathAnimation);
    return d->target;
}

void QSGPathAnimation::setTarget(QSGItem *target)
{
    Q_D(QSGPathAnimation);
    if (d->target == target)
        return;

    d->target = target;
    emit targetChanged();
}

QSGPathAnimation::Orientation QSGPathAnimation::orientation() const
{
    Q_D(const QSGPathAnimation);
    return d->orientation;
}

void QSGPathAnimation::setOrientation(Orientation orientation)
{
    Q_D(QSGPathAnimation);
    if (d->orientation == orientation)
        return;

    d->orientation = orientation;
    emit orientationChanged(d->orientation);
}

QPointF QSGPathAnimation::anchorPoint() const
{
    Q_D(const QSGPathAnimation);
    return d->anchorPoint;
}

void QSGPathAnimation::setAnchorPoint(const QPointF &point)
{
    Q_D(QSGPathAnimation);
    if (d->anchorPoint == point)
        return;

    d->anchorPoint = point;
    emit anchorPointChanged(point);
}

qreal QSGPathAnimation::orientationEntryInterval() const
{
    Q_D(const QSGPathAnimation);
    return d->entryInterval;
}

void QSGPathAnimation::setOrientationEntryInterval(qreal interval)
{
    Q_D(QSGPathAnimation);
    if (d->entryInterval == interval)
        return;
    d->entryInterval = interval;
    emit orientationEntryIntervalChanged(interval);
}

qreal QSGPathAnimation::orientationExitInterval() const
{
    Q_D(const QSGPathAnimation);
    return d->exitInterval;
}

void QSGPathAnimation::setOrientationExitInterval(qreal interval)
{
    Q_D(QSGPathAnimation);
    if (d->exitInterval == interval)
        return;
    d->exitInterval = interval;
    emit orientationExitIntervalChanged(interval);
}

qreal QSGPathAnimation::endRotation() const
{
    Q_D(const QSGPathAnimation);
    return d->endRotation.isNull ? qreal(0) : d->endRotation.value;
}

void QSGPathAnimation::setEndRotation(qreal rotation)
{
    Q_D(QSGPathAnimation);
    if (!d->endRotation.isNull && d->endRotation == rotation)
        return;

    d->endRotation = rotation;
    emit endRotationChanged(d->endRotation);
}

QAbstractAnimation2Pointer QSGPathAnimation::transition(QDeclarativeStateActions &actions,
                                           QDeclarativeProperties &modified,
                                           TransitionDirection direction)
{
    Q_D(QSGPathAnimation);
    QSGPathAnimationUpdater *data = new QSGPathAnimationUpdater;

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
            QSGPathAnimationUpdater *prevData = static_cast<QSGPathAnimationUpdater*>(d->pa->getAnimValue());

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

void QSGPathAnimationUpdater::setValue(qreal v)
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
    bool fixed = orientation == QSGPathAnimation::Fixed;
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
    QDeclarativePropertyPrivate::write(QDeclarativeProperty(target, "x"), currentPos.x(), QDeclarativePropertyPrivate::BypassInterceptor | QDeclarativePropertyPrivate::DontRemoveBinding);
    QDeclarativePropertyPrivate::write(QDeclarativeProperty(target, "y"), currentPos.y(), QDeclarativePropertyPrivate::BypassInterceptor | QDeclarativePropertyPrivate::DontRemoveBinding);

    //adjust angle according to orientation
    if (!fixed) {
        switch (orientation) {
            case QSGPathAnimation::RightFirst:
                angle = -angle;
                break;
            case QSGPathAnimation::TopFirst:
                angle = -angle + 90;
                break;
            case QSGPathAnimation::LeftFirst:
                angle = -angle + 180;
                break;
            case QSGPathAnimation::BottomFirst:
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
        QDeclarativePropertyPrivate::write(QDeclarativeProperty(target, "rotation"), angle, QDeclarativePropertyPrivate::BypassInterceptor | QDeclarativePropertyPrivate::DontRemoveBinding);
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
