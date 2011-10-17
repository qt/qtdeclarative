// Commit: e39a2e39451bf106a9845f8a60fc571faaa4dde5
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

#ifndef QSGANIMATION2_H
#define QSGANIMATION2_H

#include "qsgitem.h"

#include <private/qdeclarativeanimation_p.h>

#include "private/qabstractanimation2_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGParentAnimationPrivate;
class QSGParentAnimation : public QDeclarativeAnimationGroup
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSGParentAnimation)

    Q_PROPERTY(QSGItem *target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QSGItem *newParent READ newParent WRITE setNewParent NOTIFY newParentChanged)
    Q_PROPERTY(QSGItem *via READ via WRITE setVia NOTIFY viaChanged)

public:
    QSGParentAnimation(QObject *parent=0);
    virtual ~QSGParentAnimation();

    QSGItem *target() const;
    void setTarget(QSGItem *);

    QSGItem *newParent() const;
    void setNewParent(QSGItem *);

    QSGItem *via() const;
    void setVia(QSGItem *);

Q_SIGNALS:
    void targetChanged();
    void newParentChanged();
    void viaChanged();

protected:
    virtual QAbstractAnimation2Pointer transition(QDeclarativeStateActions &actions,
                            QDeclarativeProperties &modified,
                            TransitionDirection direction);
};

class QSGAnchorAnimationPrivate;
class QSGAnchorAnimation : public QDeclarativeAbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSGAnchorAnimation)
    Q_PROPERTY(QDeclarativeListProperty<QSGItem> targets READ targets)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(QEasingCurve easing READ easing WRITE setEasing NOTIFY easingChanged)

public:
    QSGAnchorAnimation(QObject *parent=0);
    virtual ~QSGAnchorAnimation();

    QDeclarativeListProperty<QSGItem> targets();

    int duration() const;
    void setDuration(int);

    QEasingCurve easing() const;
    void setEasing(const QEasingCurve &);

Q_SIGNALS:
    void durationChanged(int);
    void easingChanged(const QEasingCurve&);

protected:
    virtual QAbstractAnimation2Pointer transition(QDeclarativeStateActions &actions,
                            QDeclarativeProperties &modified,
                            TransitionDirection direction);
};

class QSGItem;
class QDeclarativePath;
class QSGPathAnimationPrivate;
class Q_AUTOTEST_EXPORT QSGPathAnimation : public QDeclarativeAbstractAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSGPathAnimation)

    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(QEasingCurve easing READ easing WRITE setEasing NOTIFY easingChanged)
    Q_PROPERTY(QDeclarativePath *path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QSGItem *target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(QPointF anchorPoint READ anchorPoint WRITE setAnchorPoint NOTIFY anchorPointChanged)
    Q_PROPERTY(qreal orientationEntryInterval READ orientationEntryInterval WRITE setOrientationEntryInterval NOTIFY orientationEntryIntervalChanged)
    Q_PROPERTY(qreal orientationExitInterval READ orientationExitInterval WRITE setOrientationExitInterval NOTIFY orientationExitIntervalChanged)
    Q_PROPERTY(qreal endRotation READ endRotation WRITE setEndRotation NOTIFY endRotationChanged)

public:
    QSGPathAnimation(QObject *parent=0);
    virtual ~QSGPathAnimation();

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

    QSGItem *target() const;
    void setTarget(QSGItem *);

    Orientation orientation() const;
    void setOrientation(Orientation orientation);

    QPointF anchorPoint() const;
    void setAnchorPoint(const QPointF &point);

    qreal orientationEntryInterval() const;
    void setOrientationEntryInterval(qreal);

    qreal orientationExitInterval() const;
    void setOrientationExitInterval(qreal);

    qreal endRotation() const;
    void setEndRotation(qreal);

protected:
    virtual QAbstractAnimation2Pointer transition(QDeclarativeStateActions &actions,
                            QDeclarativeProperties &modified,
                            TransitionDirection direction);

Q_SIGNALS:
    void durationChanged(int);
    void easingChanged(const QEasingCurve &);
    void pathChanged();
    void targetChanged();
    void orientationChanged(Orientation);
    void anchorPointChanged(const QPointF &);
    void orientationEntryIntervalChanged(qreal);
    void orientationExitIntervalChanged(qreal);
    void endRotationChanged(qreal);
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QSGParentAnimation)
QML_DECLARE_TYPE(QSGAnchorAnimation)
QML_DECLARE_TYPE(QSGPathAnimation)

QT_END_HEADER

#endif // QSGANIMATION2_H
