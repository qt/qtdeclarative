// Commit: e39a2e39451bf106a9845f8a60fc571faaa4dde5
/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKANIMATION_H
#define QQUICKANIMATION_H

#include "qquickitem.h"

#include <QtQuick/private/qdeclarativeanimation_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQuickParentAnimationPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickParentAnimation : public QDeclarativeAnimationGroup
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickParentAnimation)

    Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QQuickItem *newParent READ newParent WRITE setNewParent NOTIFY newParentChanged)
    Q_PROPERTY(QQuickItem *via READ via WRITE setVia NOTIFY viaChanged)

public:
    QQuickParentAnimation(QObject *parent=0);
    virtual ~QQuickParentAnimation();

    QQuickItem *target() const;
    void setTarget(QQuickItem *);

    QQuickItem *newParent() const;
    void setNewParent(QQuickItem *);

    QQuickItem *via() const;
    void setVia(QQuickItem *);

Q_SIGNALS:
    void targetChanged();
    void newParentChanged();
    void viaChanged();

protected:
    virtual QAbstractAnimationJob* transition(QDeclarativeStateActions &actions,
                            QDeclarativeProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = 0);
};

class QQuickAnchorAnimationPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickAnchorAnimation : public QDeclarativeAbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickAnchorAnimation)
    Q_PROPERTY(QDeclarativeListProperty<QQuickItem> targets READ targets)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(QEasingCurve easing READ easing WRITE setEasing NOTIFY easingChanged)

public:
    QQuickAnchorAnimation(QObject *parent=0);
    virtual ~QQuickAnchorAnimation();

    QDeclarativeListProperty<QQuickItem> targets();

    int duration() const;
    void setDuration(int);

    QEasingCurve easing() const;
    void setEasing(const QEasingCurve &);

Q_SIGNALS:
    void durationChanged(int);
    void easingChanged(const QEasingCurve&);

protected:
    virtual QAbstractAnimationJob* transition(QDeclarativeStateActions &actions,
                            QDeclarativeProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = 0);
};

class QQuickItem;
class QDeclarativePath;
class QQuickPathAnimationPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickPathAnimation : public QDeclarativeAbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickPathAnimation)

    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(QEasingCurve easing READ easing WRITE setEasing NOTIFY easingChanged)
    Q_PROPERTY(QDeclarativePath *path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(QPointF anchorPoint READ anchorPoint WRITE setAnchorPoint NOTIFY anchorPointChanged)
    Q_PROPERTY(int orientationEntryDuration READ orientationEntryDuration WRITE setOrientationEntryDuration NOTIFY orientationEntryDurationChanged)
    Q_PROPERTY(int orientationExitDuration READ orientationExitDuration WRITE setOrientationExitDuration NOTIFY orientationExitDurationChanged)
    Q_PROPERTY(qreal endRotation READ endRotation WRITE setEndRotation NOTIFY endRotationChanged)

public:
    QQuickPathAnimation(QObject *parent=0);
    virtual ~QQuickPathAnimation();

    enum Orientation {
        Fixed,
        RightFirst,
        LeftFirst,
        BottomFirst,
        TopFirst
    };
    Q_ENUMS(Orientation)

    int duration() const;
    void setDuration(int);

    QEasingCurve easing() const;
    void setEasing(const QEasingCurve &);

    QDeclarativePath *path() const;
    void setPath(QDeclarativePath *);

    QQuickItem *target() const;
    void setTarget(QQuickItem *);

    Orientation orientation() const;
    void setOrientation(Orientation orientation);

    QPointF anchorPoint() const;
    void setAnchorPoint(const QPointF &point);

    int orientationEntryDuration() const;
    void setOrientationEntryDuration(int);

    int orientationExitDuration() const;
    void setOrientationExitDuration(int);

    qreal endRotation() const;
    void setEndRotation(qreal);

protected:
    virtual QAbstractAnimationJob* transition(QDeclarativeStateActions &actions,
                            QDeclarativeProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = 0);
Q_SIGNALS:
    void durationChanged(int);
    void easingChanged(const QEasingCurve &);
    void pathChanged();
    void targetChanged();
    void orientationChanged(Orientation);
    void anchorPointChanged(const QPointF &);
    void orientationEntryDurationChanged(qreal);
    void orientationExitDurationChanged(qreal);
    void endRotationChanged(qreal);
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickParentAnimation)
QML_DECLARE_TYPE(QQuickAnchorAnimation)
QML_DECLARE_TYPE(QQuickPathAnimation)

QT_END_HEADER

#endif // QQUICKANIMATION_H
