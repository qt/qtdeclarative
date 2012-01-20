/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVEANIMATION_P_H
#define QDECLARATIVEANIMATION_P_H

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

#include "QtQuick1/private/qdeclarativeanimation_p.h"

#include "QtDeclarative/private/qdeclarativenullablevalue_p_p.h"
#include "QtQuick1/private/qdeclarativetimeline_p_p.h"

#include <QtDeclarative/qdeclarative.h>
#include <QtQuick1/qdeclarativeitem.h>
#include <QtDeclarative/qdeclarativecontext.h>

#include <QtCore/QPauseAnimation>
#include <QtCore/QVariantAnimation>
#include <QtCore/QAnimationGroup>
#include <QDebug>

#include <private/qobject_p.h>
#include <private/qvariantanimation_p.h>

QT_BEGIN_NAMESPACE

//interface for classes that provide animation actions for QActionAnimation_1
class QAbstractAnimationAction
{
public:
    virtual ~QAbstractAnimationAction() {}
    virtual void doAction() = 0;
};

//templated animation action
//allows us to specify an action that calls a function of a class.
//(so that class doesn't have to inherit QDeclarative1AbstractAnimationAction)
template<class T, void (T::*method)()>
class QAnimationActionProxy_1 : public QAbstractAnimationAction
{
public:
    QAnimationActionProxy_1(T *p) : m_p(p) {}
    virtual void doAction() { (m_p->*method)(); }

private:
    T *m_p;
};

//performs an action of type QAbstractAnimationAction
class Q_AUTOTEST_EXPORT QActionAnimation_1 : public QAbstractAnimation
{
    Q_OBJECT
public:
    QActionAnimation_1(QObject *parent = 0) : QAbstractAnimation(parent), animAction(0), policy(KeepWhenStopped) {}
    QActionAnimation_1(QAbstractAnimationAction *action, QObject *parent = 0)
        : QAbstractAnimation(parent), animAction(action), policy(KeepWhenStopped) {}
    ~QActionAnimation_1() { if (policy == DeleteWhenStopped) { delete animAction; animAction = 0; } }
    virtual int duration() const { return 0; }
    void setAnimAction(QAbstractAnimationAction *action, DeletionPolicy p)
    {
        if (state() == Running)
            stop();
        if (policy == DeleteWhenStopped)
            delete animAction;
        animAction = action;
        policy = p;
    }
protected:
    virtual void updateCurrentTime(int) {}

    virtual void updateState(State newState, State /*oldState*/)
    {
        if (newState == Running) {
            if (animAction) {
                animAction->doAction();
                if (state() == Stopped && policy == DeleteWhenStopped) {
                    delete animAction;
                    animAction = 0;
                }
            }
        }
    }

private:
    QAbstractAnimationAction *animAction;
    DeletionPolicy policy;
};

class QDeclarative1BulkValueUpdater
{
public:
    virtual ~QDeclarative1BulkValueUpdater() {}
    virtual void setValue(qreal value) = 0;
};

//animates QDeclarative1BulkValueUpdater (assumes start and end values will be reals or compatible)
class Q_AUTOTEST_EXPORT QDeclarative1BulkValueAnimator : public QVariantAnimation
{
    Q_OBJECT
public:
    QDeclarative1BulkValueAnimator(QObject *parent = 0) : QVariantAnimation(parent), animValue(0), fromSourced(0), policy(KeepWhenStopped) {}
    ~QDeclarative1BulkValueAnimator() { if (policy == DeleteWhenStopped) { delete animValue; animValue = 0; } }
    void setAnimValue(QDeclarative1BulkValueUpdater *value, DeletionPolicy p)
    {
        if (state() == Running)
            stop();
        if (policy == DeleteWhenStopped)
            delete animValue;
        animValue = value;
        policy = p;
    }
    void setFromSourcedValue(bool *value)
    {
        fromSourced = value;
    }
protected:
    virtual void updateCurrentValue(const QVariant &value)
    {
        if (state() == QAbstractAnimation::Stopped)
            return;

        if (animValue)
            animValue->setValue(value.toReal());
    }
    virtual void updateState(State newState, State oldState)
    {   
        QVariantAnimation::updateState(newState, oldState);
        if (newState == Running) {
            //check for new from every loop
            if (fromSourced)
                *fromSourced = false;
        }
    }

private:
    QDeclarative1BulkValueUpdater *animValue;
    bool *fromSourced;
    DeletionPolicy policy;
};

//an animation that just gives a tick
template<class T, void (T::*method)(int)>
class QTickAnimationProxy_1 : public QAbstractAnimation
{
    //Q_OBJECT //doesn't work with templating
public:
    QTickAnimationProxy_1(T *p, QObject *parent = 0) : QAbstractAnimation(parent), m_p(p) {}
    virtual int duration() const { return -1; }
protected:
    virtual void updateCurrentTime(int msec) { (m_p->*method)(msec); }

private:
    T *m_p;
};

class QDeclarative1AbstractAnimationPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1AbstractAnimation)
public:
    QDeclarative1AbstractAnimationPrivate()
    : running(false), paused(false), alwaysRunToEnd(false),
      connectedTimeLine(false), componentComplete(true),
      avoidPropertyValueSourceStart(false), disableUserControl(false),
      registered(false), loopCount(1), group(0) {}

    bool running:1;
    bool paused:1;
    bool alwaysRunToEnd:1;
    bool connectedTimeLine:1;
    bool componentComplete:1;
    bool avoidPropertyValueSourceStart:1;
    bool disableUserControl:1;
    bool registered:1;

    int loopCount;

    void commence();

    QDeclarativeProperty defaultProperty;

    QDeclarative1AnimationGroup *group;

    static QDeclarativeProperty createProperty(QObject *obj, const QString &str, QObject *infoObj);
};

class QDeclarative1PauseAnimationPrivate : public QDeclarative1AbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1PauseAnimation)
public:
    QDeclarative1PauseAnimationPrivate()
    : QDeclarative1AbstractAnimationPrivate(), pa(0) {}

    void init();

    QPauseAnimation *pa;
};

class QDeclarative1ScriptActionPrivate : public QDeclarative1AbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1ScriptAction)
public:
    QDeclarative1ScriptActionPrivate();

    void init();

    QDeclarativeScriptString script;
    QString name;
    QDeclarativeScriptString runScriptScript;
    bool hasRunScriptScript;
    bool reversing;

    void execute();

    QAnimationActionProxy_1<QDeclarative1ScriptActionPrivate,
                  &QDeclarative1ScriptActionPrivate::execute> proxy;
    QActionAnimation_1 *rsa;
};

class QDeclarative1PropertyActionPrivate : public QDeclarative1AbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1PropertyAction)
public:
    QDeclarative1PropertyActionPrivate()
    : QDeclarative1AbstractAnimationPrivate(), target(0), spa(0) {}

    void init();

    QObject *target;
    QString propertyName;
    QString properties;
    QList<QObject *> targets;
    QList<QObject *> exclude;

    QDeclarativeNullableValue<QVariant> value;

    QActionAnimation_1 *spa;
};

class QDeclarative1AnimationGroupPrivate : public QDeclarative1AbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1AnimationGroup)
public:
    QDeclarative1AnimationGroupPrivate()
    : QDeclarative1AbstractAnimationPrivate(), ag(0) {}

    static void append_animation(QDeclarativeListProperty<QDeclarative1AbstractAnimation> *list, QDeclarative1AbstractAnimation *role);
    static void clear_animation(QDeclarativeListProperty<QDeclarative1AbstractAnimation> *list);
    QList<QDeclarative1AbstractAnimation *> animations;
    QAnimationGroup *ag;
};

class QDeclarative1PropertyAnimationPrivate : public QDeclarative1AbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1PropertyAnimation)
public:
    QDeclarative1PropertyAnimationPrivate()
    : QDeclarative1AbstractAnimationPrivate(), target(0), fromSourced(false), fromIsDefined(false), toIsDefined(false),
      rangeIsSet(false), defaultToInterpolatorType(0), interpolatorType(0), interpolator(0), va(0), actions(0) {}

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
    bool rangeIsSet:1;
    bool defaultToInterpolatorType:1;
    int interpolatorType;
    QVariantAnimation::Interpolator interpolator;

    QDeclarative1BulkValueAnimator *va;

    // for animations that don't use the QDeclarative1BulkValueAnimator
    QDeclarative1StateActions *actions;

    static QVariant interpolateVariant(const QVariant &from, const QVariant &to, qreal progress);
    static void convertVariant(QVariant &variant, int type);
};

class QDeclarative1RotationAnimationPrivate : public QDeclarative1PropertyAnimationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1RotationAnimation)
public:
    QDeclarative1RotationAnimationPrivate() : direction(QDeclarative1RotationAnimation::Numerical) {}

    QDeclarative1RotationAnimation::RotationDirection direction;
};

class QDeclarative1ParentAnimationPrivate : public QDeclarative1AnimationGroupPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1ParentAnimation)
public:
    QDeclarative1ParentAnimationPrivate()
    : QDeclarative1AnimationGroupPrivate(), target(0), newParent(0),
       via(0), topLevelGroup(0), startAction(0), endAction(0) {}

    QDeclarativeItem *target;
    QDeclarativeItem *newParent;
    QDeclarativeItem *via;

    QSequentialAnimationGroup *topLevelGroup;
    QActionAnimation_1 *startAction;
    QActionAnimation_1 *endAction;

    QPointF computeTransformOrigin(QDeclarativeItem::TransformOrigin origin, qreal width, qreal height) const;
};

class QDeclarative1AnchorAnimationPrivate : public QDeclarative1AbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1AnchorAnimation)
public:
    QDeclarative1AnchorAnimationPrivate() : rangeIsSet(false), va(0),
        interpolator(QVariantAnimationPrivate::getInterpolator(QMetaType::QReal)) {}

    bool rangeIsSet;
    QDeclarative1BulkValueAnimator *va;
    QVariantAnimation::Interpolator interpolator;
    QList<QDeclarativeItem*> targets;
};

class Q_AUTOTEST_EXPORT QDeclarative1AnimationPropertyUpdater : public QDeclarative1BulkValueUpdater
{
public:
    QDeclarative1StateActions actions;
    int interpolatorType;       //for Number/ColorAnimation
    int prevInterpolatorType;   //for generic
    QVariantAnimation::Interpolator interpolator;
    bool reverse;
    bool fromSourced;
    bool fromDefined;
    bool *wasDeleted;
    QDeclarative1AnimationPropertyUpdater() : prevInterpolatorType(0), wasDeleted(0) {}
    ~QDeclarative1AnimationPropertyUpdater() { if (wasDeleted) *wasDeleted = true; }
    void setValue(qreal v);
};

QT_END_NAMESPACE

#endif // QDECLARATIVEANIMATION_P_H
