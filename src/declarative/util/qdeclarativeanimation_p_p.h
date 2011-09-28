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

#ifndef QDECLARATIVEANIMATION2_P_H
#define QDECLARATIVEANIMATION2_P_H

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

#include "private/qdeclarativeanimation_p.h"

#include "private/qdeclarativenullablevalue_p_p.h"
#include "private/qdeclarativetimeline_p_p.h"

#include <qdeclarative.h>
#include <qdeclarativecontext.h>

#include <private/qvariantanimation_p.h>
#include "private/qpauseanimation2_p.h"
#include <QDebug>

#include <private/qobject_p.h>
#include "private/qanimationgroup2_p.h"
#include <QDebug>

#include <private/qobject_p.h>


QT_BEGIN_NAMESPACE

//interface for classes that provide animation actions for QActionAnimation
class QAbstractAnimationAction
{
public:
    virtual ~QAbstractAnimationAction() {}
    virtual void doAction() = 0;
};

//templated animation action
//allows us to specify an action that calls a function of a class.
//(so that class doesn't have to inherit QDeclarativeAbstractAnimationAction)
template<class T, void (T::*method)()>
class QAnimationActionProxy : public QAbstractAnimationAction
{
public:
    QAnimationActionProxy(T *instance) : m_instance(instance) {}
    virtual void doAction() { (m_instance->*method)(); }

private:
    T *m_instance;
};

//performs an action of type QAbstractAnimationAction
class Q_AUTOTEST_EXPORT QActionAnimation : public QAbstractAnimation2
{
public:
    QActionAnimation(QDeclarativeAbstractAnimation *animation = 0);
    QActionAnimation(QAbstractAnimationAction *action, QDeclarativeAbstractAnimation *animation = 0);
    ~QActionAnimation();

    virtual int duration() const;
    void setAnimAction(QAbstractAnimationAction *action, DeletionPolicy p);

protected:
    virtual void updateCurrentTime(int);
    virtual void updateState(State newState, State oldState);

private:
    QAbstractAnimationAction *animAction;
    DeletionPolicy policy;
};

class QDeclarativeBulkValueUpdater
{
public:
    virtual ~QDeclarativeBulkValueUpdater() {}
    virtual void setValue(qreal value) = 0;
};

//animates QDeclarativeBulkValueUpdater (assumes start and end values will be reals or compatible)
class Q_AUTOTEST_EXPORT QDeclarativeBulkValueAnimator : public QAbstractAnimation2
{
public:
    QDeclarativeBulkValueAnimator(QDeclarativeAbstractAnimation *animation = 0);
    ~QDeclarativeBulkValueAnimator();

    void setAnimValue(QDeclarativeBulkValueUpdater *value, DeletionPolicy p);
    QDeclarativeBulkValueUpdater *getAnimValue() const { return animValue; }

    void setFromSourcedValue(bool *value) { fromSourced = value; }

    int duration() const { return m_duration; }
    void setDuration(int msecs) { m_duration = msecs; }

    QEasingCurve easingCurve() const { return easing; }
    void setEasingCurve(const QEasingCurve &curve) { easing = curve; }

protected:
    void updateCurrentTime(int currentTime);
    void updateState(State newState, State oldState);

private:
    QDeclarativeBulkValueUpdater *animValue;
    bool *fromSourced;
    DeletionPolicy policy;
    int m_duration;
    QEasingCurve easing;
};

//an animation that just gives a tick
template<class T, void (T::*method)(int)>
class QTickAnimationProxy : public QAbstractAnimation2
{
public:
    QTickAnimationProxy(T *instance, QDeclarativeAbstractAnimation *animation = 0) : QAbstractAnimation2(animation), m_instance(instance) {}
    virtual int duration() const { return -1; }
protected:
    virtual void updateCurrentTime(int msec) { (m_instance->*method)(msec); }

private:
    T *m_instance;
};

class QDeclarativeAbstractAnimationPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeAbstractAnimation)
public:
    QDeclarativeAbstractAnimationPrivate()
    : running(false), paused(false), alwaysRunToEnd(false),
      /*connectedTimeLine(false), */componentComplete(true),
      avoidPropertyValueSourceStart(false), disableUserControl(false),
      registered(false), loopCount(1), group(0) {}

    bool running:1;
    bool paused:1;
    bool alwaysRunToEnd:1;
    //bool connectedTimeLine:1;
    bool componentComplete:1;
    bool avoidPropertyValueSourceStart:1;
    bool disableUserControl:1;
    bool registered:1;

    int loopCount;

    void commence();

    QDeclarativeProperty defaultProperty;

    QDeclarativeAnimationGroup *group;

    static QDeclarativeProperty createProperty(QObject *obj, const QString &str, QObject *infoObj);
};

class QDeclarativePauseAnimationPrivate : public QDeclarativeAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativePauseAnimation)
public:
    QDeclarativePauseAnimationPrivate()
    : QDeclarativeAbstractAnimationPrivate(), pa(0) {}

    void init();

    QPauseAnimation2 *pa;
};

class QDeclarativeScriptActionPrivate : public QDeclarativeAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeScriptAction)
public:
    QDeclarativeScriptActionPrivate();

    void init();

    QDeclarativeScriptString script;
    QString name;
    QDeclarativeScriptString runScriptScript;
    bool hasRunScriptScript;
    bool reversing;

    void execute();

    QAnimationActionProxy<QDeclarativeScriptActionPrivate,
                  &QDeclarativeScriptActionPrivate::execute> proxy;
    QActionAnimation *rsa;
};

class QDeclarativePropertyActionPrivate : public QDeclarativeAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativePropertyAction)
public:
    QDeclarativePropertyActionPrivate()
    : QDeclarativeAbstractAnimationPrivate(), target(0), spa(0) {}

    void init();

    QObject *target;
    QString propertyName;
    QString properties;
    QList<QObject *> targets;
    QList<QObject *> exclude;

    QDeclarativeNullableValue<QVariant> value;

    QActionAnimation *spa;
};

class QDeclarativeAnimationGroupPrivate : public QDeclarativeAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeAnimationGroup)
public:
    QDeclarativeAnimationGroupPrivate()
    : QDeclarativeAbstractAnimationPrivate(), ag(0) {}

    static void append_animation(QDeclarativeListProperty<QDeclarativeAbstractAnimation> *list, QDeclarativeAbstractAnimation *role);
    static void clear_animation(QDeclarativeListProperty<QDeclarativeAbstractAnimation> *list);
    QList<QDeclarativeAbstractAnimation *> animations;
    QAnimationGroup2 *ag;
};

class QDeclarativePropertyAnimationPrivate : public QDeclarativeAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativePropertyAnimation)
public:
    QDeclarativePropertyAnimationPrivate()
    : QDeclarativeAbstractAnimationPrivate(), target(0), fromSourced(false), fromIsDefined(false), toIsDefined(false),
      defaultToInterpolatorType(0), interpolatorType(0), interpolator(0),  va(0), actions(0) {}

    void init();

    QVariant from;
    QVariant to;

    QObject *target;
    QString propertyName;
    QString properties;
    QList<QObject *> targets;
    QList<QObject *> exclude;
    QString defaultProperties;

    bool fromSourced;
    bool fromIsDefined:1;
    bool toIsDefined:1;
    bool defaultToInterpolatorType:1;
    int interpolatorType;
    QVariantAnimation::Interpolator interpolator;
    QDeclarativeBulkValueAnimator *va;

    // for animations that don't use the QDeclarativeBulkValueAnimator
    QDeclarativeStateActions *actions;

    static QVariant interpolateVariant(const QVariant &from, const QVariant &to, qreal progress);
    static void convertVariant(QVariant &variant, int type);
};

class QDeclarativeRotationAnimationPrivate : public QDeclarativePropertyAnimationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeRotationAnimation)
public:
    QDeclarativeRotationAnimationPrivate() : direction(QDeclarativeRotationAnimation::Numerical) {}

    QDeclarativeRotationAnimation::RotationDirection direction;
};

class Q_AUTOTEST_EXPORT QDeclarativeAnimationPropertyUpdater : public QDeclarativeBulkValueUpdater
{
public:
    QDeclarativeAnimationPropertyUpdater() : prevInterpolatorType(0), wasDeleted(0) {}
    ~QDeclarativeAnimationPropertyUpdater() { if (wasDeleted) *wasDeleted = true; }

    void setValue(qreal v);

    QDeclarativeStateActions actions;
    int interpolatorType;       //for Number/ColorAnimation
    QVariantAnimation::Interpolator interpolator;
    int prevInterpolatorType;   //for generic
    bool reverse;
    bool fromSourced;
    bool fromDefined;
    bool *wasDeleted;
};

QT_END_NAMESPACE

#endif // QDECLARATIVEANIMATION2_P_H
