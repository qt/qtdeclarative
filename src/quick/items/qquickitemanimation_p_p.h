/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQUICKANIMATION_P_H
#define QQUICKANIMATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qquickitemanimation_p.h"

#if QT_CONFIG(quick_path)
#include <private/qquickpath_p.h>
#endif
#include <private/qquickanimation_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickParentAnimationPrivate : public QQuickAnimationGroupPrivate
{
    Q_DECLARE_PUBLIC(QQuickParentAnimation)
public:
 QQuickParentAnimationPrivate()
    : QQuickAnimationGroupPrivate(), target(nullptr), newParent(nullptr), via(nullptr) {}

    QQuickItem *target;
    QQuickItem *newParent;
    QQuickItem *via;

    QPointF computeTransformOrigin(QQuickItem::TransformOrigin origin, qreal width, qreal height) const;
};

class QQuickAnchorAnimationPrivate : public QQuickAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QQuickAnchorAnimation)
public:
    QQuickAnchorAnimationPrivate() : interpolator(QVariantAnimationPrivate::getInterpolator(QMetaType::QReal)), duration(250) {}

    QVariantAnimation::Interpolator interpolator;
    int duration;
    QEasingCurve easing;
    QList<QQuickItem*> targets;
};

#if QT_CONFIG(quick_path)

class QQuickPathAnimationUpdater : public QQuickBulkValueUpdater
{
public:
    QQuickPathAnimationUpdater() : path(nullptr), pathLength(0), target(nullptr), reverse(false),
        fromSourced(false), fromDefined(false), toDefined(false),
        toX(0), toY(0), currentV(0), orientation(QQuickPathAnimation::Fixed),
        entryInterval(0), exitInterval(0) {}
    ~QQuickPathAnimationUpdater() {}

    void setValue(qreal v) override;

    QQuickPath *path;

    QPainterPath painterPath;
    QQuickCachedBezier prevBez;
    qreal pathLength;
    QList<QQuickPath::AttributePoint> attributePoints;

    QQuickItem *target;
    bool reverse;
    bool fromSourced;
    bool fromDefined;
    bool toDefined;
    qreal toX;
    qreal toY;
    qreal currentV;
    QQmlNullableValue<qreal> interruptStart;
    //TODO: bundle below into common struct
    QQuickPathAnimation::Orientation orientation;
    QPointF anchorPoint;
    qreal entryInterval;
    qreal exitInterval;
    QQmlNullableValue<qreal> endRotation;
    QQmlNullableValue<qreal> startRotation;
};

class QQuickPathAnimationPrivate;
class QQuickPathAnimationAnimator : public QQuickBulkValueAnimator
{
public:
    QQuickPathAnimationAnimator(QQuickPathAnimationPrivate * = nullptr);
    ~QQuickPathAnimationAnimator();

    void clearTemplate() { animationTemplate = nullptr; }

    QQuickPathAnimationUpdater *pathUpdater() const { return static_cast<QQuickPathAnimationUpdater*>(getAnimValue()); }
private:
    QQuickPathAnimationPrivate *animationTemplate;
};

class QQuickPathAnimationPrivate : public QQuickAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QQuickPathAnimation)
public:
    QQuickPathAnimationPrivate() : path(nullptr), target(nullptr),
        orientation(QQuickPathAnimation::Fixed), entryDuration(0), exitDuration(0), duration(250) {}

    QQuickPath *path;
    QQuickItem *target;
    QQuickPathAnimation::Orientation orientation;
    QPointF anchorPoint;
    qreal entryDuration;
    qreal exitDuration;
    QQmlNullableValue<qreal> endRotation;
    int duration;
    QEasingCurve easingCurve;
    QHash<QQuickItem*, QQuickPathAnimationAnimator* > activeAnimations;
};

#endif

QT_END_NAMESPACE

#endif // QQUICKANIMATION_P_H
