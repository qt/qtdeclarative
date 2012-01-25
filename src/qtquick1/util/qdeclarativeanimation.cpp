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

#include "QtQuick1/private/qdeclarativeanimation_p.h"
#include "QtQuick1/private/qdeclarativeanimation_p_p.h"

#include "QtQuick1/private/qdeclarativebehavior_p.h"
#include "QtQuick1/private/qdeclarativestateoperations_p.h"
#include "QtDeclarative/private/qdeclarativecontext_p.h"

#include <QtDeclarative/qdeclarativepropertyvaluesource.h>
#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtDeclarative/private/qdeclarativestringconverters_p.h>
#include <QtDeclarative/private/qdeclarativeglobal_p.h>
#include <QtDeclarative/private/qdeclarativemetatype_p.h>
#include <QtDeclarative/private/qdeclarativevaluetype_p.h>
#include <QtDeclarative/private/qdeclarativeproperty_p.h>
#include <QtDeclarative/private/qdeclarativeengine_p.h>

#include <qvariant.h>
#include <qcolor.h>
#include <qfile.h>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QtCore/qset.h>
#include <QtCore/qrect.h>
#include <QtCore/qpoint.h>
#include <QtCore/qsize.h>
#include <QtCore/qmath.h>

#include <private/qvariantanimation_p.h>

QT_BEGIN_NAMESPACE



/*!
    \qmlclass Animation QDeclarative1AbstractAnimation
    \inqmlmodule QtQuick 1
    \ingroup qml-animation-transition
    \since QtQuick 1.0
    \brief The Animation element is the base of all QML animations.

    The Animation element cannot be used directly in a QML file.  It exists
    to provide a set of common properties and methods, available across all the
    other animation types that inherit from it.  Attempting to use the Animation
    element directly will result in an error.
*/

QDeclarative1AbstractAnimation::QDeclarative1AbstractAnimation(QObject *parent)
: QObject(*(new QDeclarative1AbstractAnimationPrivate), parent)
{
}

QDeclarative1AbstractAnimation::~QDeclarative1AbstractAnimation()
{
}

QDeclarative1AbstractAnimation::QDeclarative1AbstractAnimation(QDeclarative1AbstractAnimationPrivate &dd, QObject *parent)
: QObject(dd, parent)
{
}

/*!
    \qmlproperty bool QtQuick1::Animation::running
    This property holds whether the animation is currently running.

    The \c running property can be set to declaratively control whether or not
    an animation is running.  The following example will animate a rectangle
    whenever the \l MouseArea is pressed.

    \code
    Rectangle {
        width: 100; height: 100
        NumberAnimation on x {
            running: myMouse.pressed
            from: 0; to: 100
        }
        MouseArea { id: myMouse }
    }
    \endcode

    Likewise, the \c running property can be read to determine if the animation
    is running.  In the following example the text element will indicate whether
    or not the animation is running.

    \code
    NumberAnimation { id: myAnimation }
    Text { text: myAnimation.running ? "Animation is running" : "Animation is not running" }
    \endcode

    Animations can also be started and stopped imperatively from JavaScript
    using the \c start() and \c stop() methods.

    By default, animations are not running. Though, when the animations are assigned to properties,
    as property value sources using the \e on syntax, they are set to running by default.
*/
bool QDeclarative1AbstractAnimation::isRunning() const
{
    Q_D(const QDeclarative1AbstractAnimation);
    return d->running;
}

// the behavior calls this function
void QDeclarative1AbstractAnimation::notifyRunningChanged(bool running)
{
    Q_D(QDeclarative1AbstractAnimation);
    if (d->disableUserControl && d->running != running) {
        d->running = running;
        emit runningChanged(running);
    }
}

//commence is called to start an animation when it is used as a
//simple animation, and not as part of a transition
void QDeclarative1AbstractAnimationPrivate::commence()
{
    Q_Q(QDeclarative1AbstractAnimation);

    QDeclarative1StateActions actions;
    QDeclarativeProperties properties;
    q->transition(actions, properties, QDeclarative1AbstractAnimation::Forward);

    q->qtAnimation()->start();
    if (q->qtAnimation()->state() == QAbstractAnimation::Stopped) {
        running = false;
        emit q->completed();
    }
}

QDeclarativeProperty QDeclarative1AbstractAnimationPrivate::createProperty(QObject *obj, const QString &str, QObject *infoObj)
{
    QDeclarativeProperty prop(obj, str, qmlContext(infoObj));
    if (!prop.isValid()) {
        qmlInfo(infoObj) << QDeclarative1AbstractAnimation::tr("Cannot animate non-existent property \"%1\"").arg(str);
        return QDeclarativeProperty();
    } else if (!prop.isWritable()) {
        qmlInfo(infoObj) << QDeclarative1AbstractAnimation::tr("Cannot animate read-only property \"%1\"").arg(str);
        return QDeclarativeProperty();
    }
    return prop;
}

void QDeclarative1AbstractAnimation::setRunning(bool r)
{
    Q_D(QDeclarative1AbstractAnimation);
    if (!d->componentComplete) {
        d->running = r;
        if (r == false)
            d->avoidPropertyValueSourceStart = true;
        else if (!d->registered) {
            d->registered = true;
            QDeclarativeEnginePrivate *engPriv = QDeclarativeEnginePrivate::get(qmlEngine(this));
            engPriv->registerFinalizeCallback(this, this->metaObject()->indexOfSlot("componentFinalized()"));
        }
        return;
    }

    if (d->running == r)
        return;

    if (d->group || d->disableUserControl) {
        qmlInfo(this) << "setRunning() cannot be used on non-root animation nodes.";
        return;
    }

    d->running = r;
    if (d->running) {
        bool supressStart = false;
        if (d->alwaysRunToEnd && d->loopCount != 1
            && qtAnimation()->state() == QAbstractAnimation::Running) {
            //we've restarted before the final loop finished; restore proper loop count
            if (d->loopCount == -1)
                qtAnimation()->setLoopCount(d->loopCount);
            else
                qtAnimation()->setLoopCount(qtAnimation()->currentLoop() + d->loopCount);
            supressStart = true;    //we want the animation to continue, rather than restart
        }

        if (!d->connectedTimeLine) {
            QObject::connect(qtAnimation(), SIGNAL(finished()),
                             this, SLOT(timelineComplete()));
            d->connectedTimeLine = true;
        }
        if (!supressStart)
            d->commence();
        emit started();
    } else {
        if (d->alwaysRunToEnd) {
            if (d->loopCount != 1)
                qtAnimation()->setLoopCount(qtAnimation()->currentLoop()+1);    //finish the current loop
        } else
            qtAnimation()->stop();

        emit completed();
    }

    emit runningChanged(d->running);
}

/*!
    \qmlproperty bool QtQuick1::Animation::paused
    This property holds whether the animation is currently paused.

    The \c paused property can be set to declaratively control whether or not
    an animation is paused.

    Animations can also be paused and resumed imperatively from JavaScript
    using the \c pause() and \c resume() methods.

    By default, animations are not paused.
*/
bool QDeclarative1AbstractAnimation::isPaused() const
{
    Q_D(const QDeclarative1AbstractAnimation);
    return d->paused;
}

void QDeclarative1AbstractAnimation::setPaused(bool p)
{
    Q_D(QDeclarative1AbstractAnimation);
    if (d->paused == p)
        return;

    if (d->group || d->disableUserControl) {
        qmlInfo(this) << "setPaused() cannot be used on non-root animation nodes.";
        return;
    }

    d->paused = p;
    if (d->paused)
        qtAnimation()->pause();
    else
        qtAnimation()->resume();

    emit pausedChanged(d->paused);
}

void QDeclarative1AbstractAnimation::classBegin()
{
    Q_D(QDeclarative1AbstractAnimation);
    d->componentComplete = false;
}

void QDeclarative1AbstractAnimation::componentComplete()
{
    Q_D(QDeclarative1AbstractAnimation);
    d->componentComplete = true;
}

void QDeclarative1AbstractAnimation::componentFinalized()
{
    Q_D(QDeclarative1AbstractAnimation);
    if (d->running) {
        d->running = false;
        setRunning(true);
    }
}

/*!
    \qmlproperty bool QtQuick1::Animation::alwaysRunToEnd
    This property holds whether the animation should run to completion when it is stopped.

    If this true the animation will complete its current iteration when it
    is stopped - either by setting the \c running property to false, or by
    calling the \c stop() method.  The \c complete() method is not effected
    by this value.

    This behavior is most useful when the \c repeat property is set, as the
    animation will finish playing normally but not restart.

    By default, the alwaysRunToEnd property is not set.

    \note alwaysRunToEnd has no effect on animations in a Transition.
*/
bool QDeclarative1AbstractAnimation::alwaysRunToEnd() const
{
    Q_D(const QDeclarative1AbstractAnimation);
    return d->alwaysRunToEnd;
}

void QDeclarative1AbstractAnimation::setAlwaysRunToEnd(bool f)
{
    Q_D(QDeclarative1AbstractAnimation);
    if (d->alwaysRunToEnd == f)
        return;

    d->alwaysRunToEnd = f;
    emit alwaysRunToEndChanged(f);
}

/*!
    \qmlproperty int QtQuick1::Animation::loops
    This property holds the number of times the animation should play.

    By default, \c loops is 1: the animation will play through once and then stop.

    If set to Animation.Infinite, the animation will continuously repeat until it is explicitly
    stopped - either by setting the \c running property to false, or by calling
    the \c stop() method.

    In the following example, the rectangle will spin indefinitely.

    \code
    Rectangle {
        width: 100; height: 100; color: "green"
        RotationAnimation on rotation {
            loops: Animation.Infinite
            from: 0
            to: 360
        }
    }
    \endcode
*/
int QDeclarative1AbstractAnimation::loops() const
{
    Q_D(const QDeclarative1AbstractAnimation);
    return d->loopCount;
}

void QDeclarative1AbstractAnimation::setLoops(int loops)
{
    Q_D(QDeclarative1AbstractAnimation);
    if (loops < 0)
        loops = -1;

    if (loops == d->loopCount)
        return;

    d->loopCount = loops;
    qtAnimation()->setLoopCount(loops);
    emit loopCountChanged(loops);
}


int QDeclarative1AbstractAnimation::currentTime()
{
    return qtAnimation()->currentLoopTime();
}

void QDeclarative1AbstractAnimation::setCurrentTime(int time)
{
    qtAnimation()->setCurrentTime(time);
}

QDeclarative1AnimationGroup *QDeclarative1AbstractAnimation::group() const
{
    Q_D(const QDeclarative1AbstractAnimation);
    return d->group;
}

void QDeclarative1AbstractAnimation::setGroup(QDeclarative1AnimationGroup *g)
{
    Q_D(QDeclarative1AbstractAnimation);
    if (d->group == g)
        return;
    if (d->group)
        static_cast<QDeclarative1AnimationGroupPrivate *>(d->group->d_func())->animations.removeAll(this);

    d->group = g;

    if (d->group && !static_cast<QDeclarative1AnimationGroupPrivate *>(d->group->d_func())->animations.contains(this))
        static_cast<QDeclarative1AnimationGroupPrivate *>(d->group->d_func())->animations.append(this);

    //if (g) //if removed from a group, then the group should no longer be the parent
        setParent(g);
}

/*!
    \qmlmethod QtQuick1::Animation::start()
    \brief Starts the animation.

    If the animation is already running, calling this method has no effect.  The
    \c running property will be true following a call to \c start().
*/
void QDeclarative1AbstractAnimation::start()
{
    setRunning(true);
}

/*!
    \qmlmethod QtQuick1::Animation::pause()
    \brief Pauses the animation.

    If the animation is already paused, calling this method has no effect.  The
    \c paused property will be true following a call to \c pause().
*/
void QDeclarative1AbstractAnimation::pause()
{
    setPaused(true);
}

/*!
    \qmlmethod QtQuick1::Animation::resume()
    \brief Resumes a paused animation.

    If the animation is not paused, calling this method has no effect.  The
    \c paused property will be false following a call to \c resume().
*/
void QDeclarative1AbstractAnimation::resume()
{
    setPaused(false);
}

/*!
    \qmlmethod QtQuick1::Animation::stop()
    \brief Stops the animation.

    If the animation is not running, calling this method has no effect.  The
    \c running property will be false following a call to \c stop().

    Normally \c stop() stops the animation immediately, and the animation has
    no further influence on property values.  In this example animation
    \code
    Rectangle {
        NumberAnimation on x { from: 0; to: 100; duration: 500 }
    }
    \endcode
    was stopped at time 250ms, the \c x property will have a value of 50.

    However, if the \c alwaysRunToEnd property is set, the animation will
    continue running until it completes and then stop.  The \c running property
    will still become false immediately.
*/
void QDeclarative1AbstractAnimation::stop()
{
    setRunning(false);
}

/*!
    \qmlmethod QtQuick1::Animation::restart()
    \brief Restarts the animation.

    This is a convenience method, and is equivalent to calling \c stop() and
    then \c start().
*/
void QDeclarative1AbstractAnimation::restart()
{
    stop();
    start();
}

/*!
    \qmlmethod QtQuick1::Animation::complete()
    \brief Stops the animation, jumping to the final property values.

    If the animation is not running, calling this method has no effect.  The
    \c running property will be false following a call to \c complete().

    Unlike \c stop(), \c complete() immediately fast-forwards the animation to
    its end.  In the following example,
    \code
    Rectangle {
        NumberAnimation on x { from: 0; to: 100; duration: 500 }
    }
    \endcode
    calling \c stop() at time 250ms will result in the \c x property having
    a value of 50, while calling \c complete() will set the \c x property to
    100, exactly as though the animation had played the whole way through.
*/
void QDeclarative1AbstractAnimation::complete()
{
    if (isRunning()) {
         qtAnimation()->setCurrentTime(qtAnimation()->duration());
    }
}

void QDeclarative1AbstractAnimation::setTarget(const QDeclarativeProperty &p)
{
    Q_D(QDeclarative1AbstractAnimation);
    d->defaultProperty = p;

    if (!d->avoidPropertyValueSourceStart)
        setRunning(true);
}

/*
    we rely on setTarget only being called when used as a value source
    so this function allows us to do the same thing as setTarget without
    that assumption
*/
void QDeclarative1AbstractAnimation::setDefaultTarget(const QDeclarativeProperty &p)
{
    Q_D(QDeclarative1AbstractAnimation);
    d->defaultProperty = p;
}

/*
    don't allow start/stop/pause/resume to be manually invoked,
    because something else (like a Behavior) already has control
    over the animation.
*/
void QDeclarative1AbstractAnimation::setDisableUserControl()
{
    Q_D(QDeclarative1AbstractAnimation);
    d->disableUserControl = true;
}

void QDeclarative1AbstractAnimation::transition(QDeclarative1StateActions &actions,
                                      QDeclarativeProperties &modified,
                                      TransitionDirection direction)
{
    Q_UNUSED(actions);
    Q_UNUSED(modified);
    Q_UNUSED(direction);
}

void QDeclarative1AbstractAnimation::timelineComplete()
{
    Q_D(QDeclarative1AbstractAnimation);
    setRunning(false);
    if (d->alwaysRunToEnd && d->loopCount != 1) {
        //restore the proper loopCount for the next run
        qtAnimation()->setLoopCount(d->loopCount);
    }
}

/*!
    \qmlclass PauseAnimation QDeclarative1PauseAnimation
    \inqmlmodule QtQuick 1
    \ingroup qml-animation-transition
    \since QtQuick 1.0
    \inherits Animation
    \brief The PauseAnimation element provides a pause for an animation.

    When used in a SequentialAnimation, PauseAnimation is a step when
    nothing happens, for a specified duration.

    A 500ms animation sequence, with a 100ms pause between two animations:
    \code
    SequentialAnimation {
        NumberAnimation { ... duration: 200 }
        PauseAnimation { duration: 100 }
        NumberAnimation { ... duration: 200 }
    }
    \endcode

    \sa {QML Animation and Transitions}, {declarative/animation/basics}{Animation basics example}
*/
QDeclarative1PauseAnimation::QDeclarative1PauseAnimation(QObject *parent)
: QDeclarative1AbstractAnimation(*(new QDeclarative1PauseAnimationPrivate), parent)
{
    Q_D(QDeclarative1PauseAnimation);
    d->init();
}

QDeclarative1PauseAnimation::~QDeclarative1PauseAnimation()
{
}

void QDeclarative1PauseAnimationPrivate::init()
{
    Q_Q(QDeclarative1PauseAnimation);
    pa = new QPauseAnimation;
    QDeclarative_setParent_noEvent(pa, q);
}

/*!
    \qmlproperty int QtQuick1::PauseAnimation::duration
    This property holds the duration of the pause in milliseconds

    The default value is 250.
*/
int QDeclarative1PauseAnimation::duration() const
{
    Q_D(const QDeclarative1PauseAnimation);
    return d->pa->duration();
}

void QDeclarative1PauseAnimation::setDuration(int duration)
{
    if (duration < 0) {
        qmlInfo(this) << tr("Cannot set a duration of < 0");
        return;
    }

    Q_D(QDeclarative1PauseAnimation);
    if (d->pa->duration() == duration)
        return;
    d->pa->setDuration(duration);
    emit durationChanged(duration);
}

QAbstractAnimation *QDeclarative1PauseAnimation::qtAnimation()
{
    Q_D(QDeclarative1PauseAnimation);
    return d->pa;
}

/*!
    \qmlclass ColorAnimation QDeclarative1ColorAnimation
    \inqmlmodule QtQuick 1
  \ingroup qml-animation-transition
    \since QtQuick 1.0
    \inherits PropertyAnimation
    \brief The ColorAnimation element animates changes in color values.

    ColorAnimation is a specialized PropertyAnimation that defines an
    animation to be applied when a color value changes.

    Here is a ColorAnimation applied to the \c color property of a \l Rectangle
    as a property value source. It animates the \c color property's value from
    its current value to a value of "red", over 1000 milliseconds:

    \snippet doc/src/snippets/qtquick1/coloranimation.qml 0

    Like any other animation element, a ColorAnimation can be applied in a
    number of ways, including transitions, behaviors and property value
    sources. The \l {QML Animation and Transitions} documentation shows a
    variety of methods for creating animations.

    For convenience, when a ColorAnimation is used in a \l Transition, it will
    animate any \c color properties that have been modified during the state
    change. If a \l{PropertyAnimation::}{property} or
    \l{PropertyAnimation::}{properties} are explicitly set for the animation,
    then those are used instead.

    \sa {QML Animation and Transitions}, {declarative/animation/basics}{Animation basics example}
*/
QDeclarative1ColorAnimation::QDeclarative1ColorAnimation(QObject *parent)
: QDeclarative1PropertyAnimation(parent)
{
    Q_D(QDeclarative1PropertyAnimation);
    d->interpolatorType = QMetaType::QColor;
    d->interpolator = QVariantAnimationPrivate::getInterpolator(d->interpolatorType);
    d->defaultToInterpolatorType = true;
}

QDeclarative1ColorAnimation::~QDeclarative1ColorAnimation()
{
}

/*!
    \qmlproperty color QtQuick1::ColorAnimation::from
    This property holds the color value at which the animation should begin.

    For example, the following animation is not applied until a color value
    has reached "#c0c0c0":

    \qml
    Item {
        states: [
            // States are defined here...
        ]

        transition: Transition {
            NumberAnimation { from: "#c0c0c0"; duration: 2000 }
        }
    }
    \endqml

    If the ColorAnimation is defined within a \l Transition or \l Behavior,
    this value defaults to the value defined in the starting state of the
    \l Transition, or the current value of the property at the moment the
    \l Behavior is triggered.

    \sa {QML Animation and Transitions}
*/
QColor QDeclarative1ColorAnimation::from() const
{
    Q_D(const QDeclarative1PropertyAnimation);
    return d->from.value<QColor>();
}

void QDeclarative1ColorAnimation::setFrom(const QColor &f)
{
    QDeclarative1PropertyAnimation::setFrom(f);
}

/*!
    \qmlproperty color QtQuick1::ColorAnimation::to

    This property holds the color value at which the animation should end.

    If the ColorAnimation is defined within a \l Transition or \l Behavior,
    this value defaults to the value defined in the end state of the
    \l Transition, or the value of the property change that triggered the
    \l Behavior.

    \sa {QML Animation and Transitions}
*/
QColor QDeclarative1ColorAnimation::to() const
{
    Q_D(const QDeclarative1PropertyAnimation);
    return d->to.value<QColor>();
}

void QDeclarative1ColorAnimation::setTo(const QColor &t)
{
    QDeclarative1PropertyAnimation::setTo(t);
}



/*!
    \qmlclass ScriptAction QDeclarative1ScriptAction
    \inqmlmodule QtQuick 1
    \ingroup qml-animation-transition
    \since QtQuick 1.0
    \inherits Animation
    \brief The ScriptAction element allows scripts to be run during an animation.

    ScriptAction can be used to run a script at a specific point in an animation.

    \qml
    SequentialAnimation {
        NumberAnimation {
            // ...
        }
        ScriptAction { script: doSomething(); }
        NumberAnimation {
            // ...
        }
    }
    \endqml

    When used as part of a Transition, you can also target a specific
    StateChangeScript to run using the \c scriptName property.

    \snippet doc/src/snippets/qtquick1/states/statechangescript.qml state and transition

    \sa StateChangeScript
*/
QDeclarative1ScriptAction::QDeclarative1ScriptAction(QObject *parent)
    :QDeclarative1AbstractAnimation(*(new QDeclarative1ScriptActionPrivate), parent)
{
    Q_D(QDeclarative1ScriptAction);
    d->init();
}

QDeclarative1ScriptAction::~QDeclarative1ScriptAction()
{
}

void QDeclarative1ScriptActionPrivate::init()
{
    Q_Q(QDeclarative1ScriptAction);
    rsa = new QActionAnimation_1(&proxy);
    QDeclarative_setParent_noEvent(rsa, q);
}

/*!
    \qmlproperty script QtQuick1::ScriptAction::script
    This property holds the script to run.
*/
QDeclarativeScriptString QDeclarative1ScriptAction::script() const
{
    Q_D(const QDeclarative1ScriptAction);
    return d->script;
}

void QDeclarative1ScriptAction::setScript(const QDeclarativeScriptString &script)
{
    Q_D(QDeclarative1ScriptAction);
    d->script = script;
}

/*!
    \qmlproperty string QtQuick1::ScriptAction::scriptName
    This property holds the the name of the StateChangeScript to run.

    This property is only valid when ScriptAction is used as part of a transition.
    If both script and scriptName are set, scriptName will be used.

    \note When using scriptName in a reversible transition, the script will only
    be run when the transition is being run forwards.
*/
QString QDeclarative1ScriptAction::stateChangeScriptName() const
{
    Q_D(const QDeclarative1ScriptAction);
    return d->name;
}

void QDeclarative1ScriptAction::setStateChangeScriptName(const QString &name)
{
    Q_D(QDeclarative1ScriptAction);
    d->name = name;
}

void QDeclarative1ScriptActionPrivate::execute()
{
    Q_Q(QDeclarative1ScriptAction);
    if (hasRunScriptScript && reversing)
        return;

    QDeclarativeScriptString scriptStr = hasRunScriptScript ? runScriptScript : script;

    const QString &str = scriptStr.script();
    if (!str.isEmpty()) {
        QDeclarativeExpression expr(scriptStr.context(), scriptStr.scopeObject(), str);
        QDeclarativeData *ddata = QDeclarativeData::get(q);
        if (ddata && ddata->outerContext && !ddata->outerContext->url.isEmpty())
            expr.setSourceLocation(ddata->outerContext->url.toString(), ddata->lineNumber, ddata->columnNumber);
        expr.evaluate();
        if (expr.hasError())
            qmlInfo(q) << expr.error();
    }
}

void QDeclarative1ScriptAction::transition(QDeclarative1StateActions &actions,
                                    QDeclarativeProperties &modified,
                                    TransitionDirection direction)
{
    Q_D(QDeclarative1ScriptAction);
    Q_UNUSED(modified);

    d->hasRunScriptScript = false;
    d->reversing = (direction == Backward);
    for (int ii = 0; ii < actions.count(); ++ii) {
        QDeclarative1Action &action = actions[ii];

        if (action.event && action.event->typeName() == QLatin1String("StateChangeScript")
            && static_cast<QDeclarative1StateChangeScript*>(action.event)->name() == d->name) {
            d->runScriptScript = static_cast<QDeclarative1StateChangeScript*>(action.event)->script();
            d->hasRunScriptScript = true;
            action.actionDone = true;
            break;  //only match one (names should be unique)
        }
    }
}

QAbstractAnimation *QDeclarative1ScriptAction::qtAnimation()
{
    Q_D(QDeclarative1ScriptAction);
    return d->rsa;
}



/*!
    \qmlclass PropertyAction QDeclarative1PropertyAction
    \inqmlmodule QtQuick 1
    \ingroup qml-animation-transition
    \since QtQuick 1.0
    \inherits Animation
    \brief The PropertyAction element allows immediate property changes during animation.

    PropertyAction is used to specify an immediate property change during an
    animation. The property change is not animated.

    It is useful for setting non-animated property values during an animation.

    For example, here is a SequentialAnimation that sets the image's
    \l {Image::}{smooth} property to \c true, animates the width of the image,
    then sets \l {Image::}{smooth} back to \c false:

    \snippet doc/src/snippets/qtquick1/propertyaction.qml standalone

    PropertyAction is also useful for setting the exact point at which a property
    change should occur during a \l Transition. For example, if PropertyChanges
    was used in a \l State to rotate an item around a particular
    \l {Item::}{transformOrigin}, it might be implemented like this:

    \snippet doc/src/snippets/qtquick1/propertyaction.qml transition

    However, with this code, the \c transformOrigin is not set until \e after
    the animation, as a \l State is taken to define the values at the \e end of
    a transition. The animation would rotate at the default \c transformOrigin,
    then jump to \c Item.BottomRight. To fix this, insert a PropertyAction
    before the RotationAnimation begins:

    \snippet doc/src/snippets/qtquick1/propertyaction-sequential.qml sequential

    This immediately sets the \c transformOrigin property to the value defined
    in the end state of the \l Transition (i.e. the value defined in the
    PropertyAction object) so that the rotation animation begins with the
    correct transform origin.

    \sa {QML Animation and Transitions}, QtDeclarative
*/
QDeclarative1PropertyAction::QDeclarative1PropertyAction(QObject *parent)
: QDeclarative1AbstractAnimation(*(new QDeclarative1PropertyActionPrivate), parent)
{
    Q_D(QDeclarative1PropertyAction);
    d->init();
}

QDeclarative1PropertyAction::~QDeclarative1PropertyAction()
{
}

void QDeclarative1PropertyActionPrivate::init()
{
    Q_Q(QDeclarative1PropertyAction);
    spa = new QActionAnimation_1;
    QDeclarative_setParent_noEvent(spa, q);
}

QObject *QDeclarative1PropertyAction::target() const
{
    Q_D(const QDeclarative1PropertyAction);
    return d->target;
}

void QDeclarative1PropertyAction::setTarget(QObject *o)
{
    Q_D(QDeclarative1PropertyAction);
    if (d->target == o)
        return;
    d->target = o;
    emit targetChanged();
}

QString QDeclarative1PropertyAction::property() const
{
    Q_D(const QDeclarative1PropertyAction);
    return d->propertyName;
}

void QDeclarative1PropertyAction::setProperty(const QString &n)
{
    Q_D(QDeclarative1PropertyAction);
    if (d->propertyName == n)
        return;
    d->propertyName = n;
    emit propertyChanged();
}

/*!
    \qmlproperty Object QtQuick1::PropertyAction::target
    \qmlproperty list<Object> QtQuick1::PropertyAction::targets
    \qmlproperty string QtQuick1::PropertyAction::property
    \qmlproperty string QtQuick1::PropertyAction::properties

    These properties determine the items and their properties that are
    affected by this action.

    The details of how these properties are interpreted in different situations
    is covered in the \l{PropertyAnimation::properties}{corresponding} PropertyAnimation
    documentation.

    \sa exclude
*/
QString QDeclarative1PropertyAction::properties() const
{
    Q_D(const QDeclarative1PropertyAction);
    return d->properties;
}

void QDeclarative1PropertyAction::setProperties(const QString &p)
{
    Q_D(QDeclarative1PropertyAction);
    if (d->properties == p)
        return;
    d->properties = p;
    emit propertiesChanged(p);
}

QDeclarativeListProperty<QObject> QDeclarative1PropertyAction::targets()
{
    Q_D(QDeclarative1PropertyAction);
    return QDeclarativeListProperty<QObject>(this, d->targets);
}

/*!
    \qmlproperty list<Object> QtQuick1::PropertyAction::exclude
    This property holds the objects that should not be affected by this action.

    \sa targets
*/
QDeclarativeListProperty<QObject> QDeclarative1PropertyAction::exclude()
{
    Q_D(QDeclarative1PropertyAction);
    return QDeclarativeListProperty<QObject>(this, d->exclude);
}

/*!
    \qmlproperty any QtQuick1::PropertyAction::value
    This property holds the value to be set on the property.

    If the PropertyAction is defined within a \l Transition or \l Behavior,
    this value defaults to the value defined in the end state of the
    \l Transition, or the value of the property change that triggered the
    \l Behavior.
*/
QVariant QDeclarative1PropertyAction::value() const
{
    Q_D(const QDeclarative1PropertyAction);
    return d->value;
}

void QDeclarative1PropertyAction::setValue(const QVariant &v)
{
    Q_D(QDeclarative1PropertyAction);
    if (d->value.isNull || d->value != v) {
        d->value = v;
        emit valueChanged(v);
    }
}

QAbstractAnimation *QDeclarative1PropertyAction::qtAnimation()
{
    Q_D(QDeclarative1PropertyAction);
    return d->spa;
}

void QDeclarative1PropertyAction::transition(QDeclarative1StateActions &actions,
                                      QDeclarativeProperties &modified,
                                      TransitionDirection direction)
{
    Q_D(QDeclarative1PropertyAction);
    Q_UNUSED(direction);

    struct QDeclarative1SetPropertyAnimationAction : public QAbstractAnimationAction
    {
        QDeclarative1StateActions actions;
        virtual void doAction()
        {
            for (int ii = 0; ii < actions.count(); ++ii) {
                const QDeclarative1Action &action = actions.at(ii);
                QDeclarativePropertyPrivate::write(action.property, action.toValue, QDeclarativePropertyPrivate::BypassInterceptor | QDeclarativePropertyPrivate::DontRemoveBinding);
            }
        }
    };

    QStringList props = d->properties.isEmpty() ? QStringList() : d->properties.split(QLatin1Char(','));
    for (int ii = 0; ii < props.count(); ++ii)
        props[ii] = props.at(ii).trimmed();
    if (!d->propertyName.isEmpty())
        props << d->propertyName;

    QList<QObject*> targets = d->targets;
    if (d->target)
        targets.append(d->target);

    bool hasSelectors = !props.isEmpty() || !targets.isEmpty() || !d->exclude.isEmpty();

    if (d->defaultProperty.isValid() && !hasSelectors) {
        props << d->defaultProperty.name();
        targets << d->defaultProperty.object();
    }

    QDeclarative1SetPropertyAnimationAction *data = new QDeclarative1SetPropertyAnimationAction;

    bool hasExplicit = false;
    //an explicit animation has been specified
    if (d->value.isValid()) {
        for (int i = 0; i < props.count(); ++i) {
            for (int j = 0; j < targets.count(); ++j) {
                QDeclarative1Action myAction;
                myAction.property = d->createProperty(targets.at(j), props.at(i), this);
                if (myAction.property.isValid()) {
                    myAction.toValue = d->value;
                    QDeclarative1PropertyAnimationPrivate::convertVariant(myAction.toValue, myAction.property.propertyType());
                    data->actions << myAction;
                    hasExplicit = true;
                    for (int ii = 0; ii < actions.count(); ++ii) {
                        QDeclarative1Action &action = actions[ii];
                        if (action.property.object() == myAction.property.object() &&
                            myAction.property.name() == action.property.name()) {
                            modified << action.property;
                            break;  //### any chance there could be multiples?
                        }
                    }
                }
            }
        }
    }

    if (!hasExplicit)
    for (int ii = 0; ii < actions.count(); ++ii) {
        QDeclarative1Action &action = actions[ii];

        QObject *obj = action.property.object();
        QString propertyName = action.property.name();
        QObject *sObj = action.specifiedObject;
        QString sPropertyName = action.specifiedProperty;
        bool same = (obj == sObj);

        if ((targets.isEmpty() || targets.contains(obj) || (!same && targets.contains(sObj))) &&
           (!d->exclude.contains(obj)) && (same || (!d->exclude.contains(sObj))) &&
           (props.contains(propertyName) || (!same && props.contains(sPropertyName)))) {
            QDeclarative1Action myAction = action;

            if (d->value.isValid())
                myAction.toValue = d->value;
            QDeclarative1PropertyAnimationPrivate::convertVariant(myAction.toValue, myAction.property.propertyType());

            modified << action.property;
            data->actions << myAction;
            action.fromValue = myAction.toValue;
        }
    }

    if (data->actions.count()) {
        d->spa->setAnimAction(data, QAbstractAnimation::DeleteWhenStopped);
    } else {
        delete data;
    }
}

/*!
    \qmlclass NumberAnimation QDeclarative1NumberAnimation
    \inqmlmodule QtQuick 1
  \ingroup qml-animation-transition
    \since QtQuick 1.0
    \inherits PropertyAnimation
    \brief The NumberAnimation element animates changes in qreal-type values.

    NumberAnimation is a specialized PropertyAnimation that defines an
    animation to be applied when a numerical value changes.

    Here is a NumberAnimation applied to the \c x property of a \l Rectangle
    as a property value source. It animates the \c x value from its current
    value to a value of 50, over 1000 milliseconds:

    \snippet doc/src/snippets/qtquick1/numberanimation.qml 0

    Like any other animation element, a NumberAnimation can be applied in a
    number of ways, including transitions, behaviors and property value
    sources. The \l {QML Animation and Transitions} documentation shows a
    variety of methods for creating animations.

    Note that NumberAnimation may not animate smoothly if there are irregular
    changes in the number value that it is tracking. If this is the case, use
    SmoothedAnimation instead.

    \sa {QML Animation and Transitions}, {declarative/animation/basics}{Animation basics example}
*/
QDeclarative1NumberAnimation::QDeclarative1NumberAnimation(QObject *parent)
: QDeclarative1PropertyAnimation(parent)
{
    init();
}

QDeclarative1NumberAnimation::QDeclarative1NumberAnimation(QDeclarative1PropertyAnimationPrivate &dd, QObject *parent)
: QDeclarative1PropertyAnimation(dd, parent)
{
    init();
}

QDeclarative1NumberAnimation::~QDeclarative1NumberAnimation()
{
}

void QDeclarative1NumberAnimation::init()
{
    Q_D(QDeclarative1PropertyAnimation);
    d->interpolatorType = QMetaType::QReal;
    d->interpolator = QVariantAnimationPrivate::getInterpolator(d->interpolatorType);
}

/*!
    \qmlproperty real QtQuick1::NumberAnimation::from
    This property holds the starting value for the animation.

    For example, the following animation is not applied until the \c x value
    has reached 100:

    \qml
    Item {
        states: [
            // ...
        ]

        transition: Transition {
            NumberAnimation { properties: "x"; from: 100; duration: 200 }
        }
    }
    \endqml

    If the NumberAnimation is defined within a \l Transition or \l Behavior,
    this value defaults to the value defined in the starting state of the
    \l Transition, or the current value of the property at the moment the
    \l Behavior is triggered.

    \sa {QML Animation and Transitions}
*/

qreal QDeclarative1NumberAnimation::from() const
{
    Q_D(const QDeclarative1PropertyAnimation);
    return d->from.toReal();
}

void QDeclarative1NumberAnimation::setFrom(qreal f)
{
    QDeclarative1PropertyAnimation::setFrom(f);
}

/*!
    \qmlproperty real QtQuick1::NumberAnimation::to
    This property holds the end value for the animation.

    If the NumberAnimation is defined within a \l Transition or \l Behavior,
    this value defaults to the value defined in the end state of the
    \l Transition, or the value of the property change that triggered the
    \l Behavior.

    \sa {QML Animation and Transitions}
*/
qreal QDeclarative1NumberAnimation::to() const
{
    Q_D(const QDeclarative1PropertyAnimation);
    return d->to.toReal();
}

void QDeclarative1NumberAnimation::setTo(qreal t)
{
    QDeclarative1PropertyAnimation::setTo(t);
}



/*!
    \qmlclass Vector3dAnimation QDeclarative1Vector3dAnimation
    \inqmlmodule QtQuick 1
    \ingroup qml-animation-transition
    \since QtQuick 1.0
    \inherits PropertyAnimation
    \brief The Vector3dAnimation element animates changes in QVector3d values.

    Vector3dAnimation is a specialized PropertyAnimation that defines an
    animation to be applied when a Vector3d value changes.

    Like any other animation element, a Vector3dAnimation can be applied in a
    number of ways, including transitions, behaviors and property value
    sources. The \l {QML Animation and Transitions} documentation shows a
    variety of methods for creating animations.

    \sa {QML Animation and Transitions}, {declarative/animation/basics}{Animation basics example}
*/
QDeclarative1Vector3dAnimation::QDeclarative1Vector3dAnimation(QObject *parent)
: QDeclarative1PropertyAnimation(parent)
{
    Q_D(QDeclarative1PropertyAnimation);
    d->interpolatorType = QMetaType::QVector3D;
    d->interpolator = QVariantAnimationPrivate::getInterpolator(d->interpolatorType);
    d->defaultToInterpolatorType = true;
}

QDeclarative1Vector3dAnimation::~QDeclarative1Vector3dAnimation()
{
}

/*!
    \qmlproperty real QtQuick1::Vector3dAnimation::from
    This property holds the starting value for the animation.

    If the Vector3dAnimation is defined within a \l Transition or \l Behavior,
    this value defaults to the value defined in the starting state of the
    \l Transition, or the current value of the property at the moment the
    \l Behavior is triggered.

    \sa {QML Animation and Transitions}
*/
QVector3D QDeclarative1Vector3dAnimation::from() const
{
    Q_D(const QDeclarative1PropertyAnimation);
    return d->from.value<QVector3D>();
}

void QDeclarative1Vector3dAnimation::setFrom(QVector3D f)
{
    QDeclarative1PropertyAnimation::setFrom(f);
}

/*!
    \qmlproperty real QtQuick1::Vector3dAnimation::to
    This property holds the end value for the animation.

    If the Vector3dAnimation is defined within a \l Transition or \l Behavior,
    this value defaults to the value defined in the end state of the
    \l Transition, or the value of the property change that triggered the
    \l Behavior.

    \sa {QML Animation and Transitions}
*/
QVector3D QDeclarative1Vector3dAnimation::to() const
{
    Q_D(const QDeclarative1PropertyAnimation);
    return d->to.value<QVector3D>();
}

void QDeclarative1Vector3dAnimation::setTo(QVector3D t)
{
    QDeclarative1PropertyAnimation::setTo(t);
}



/*!
    \qmlclass RotationAnimation QDeclarative1RotationAnimation
    \inqmlmodule QtQuick 1
    \ingroup qml-animation-transition
    \since QtQuick 1.0
    \inherits PropertyAnimation
    \brief The RotationAnimation element animates changes in rotation values.

    RotationAnimation is a specialized PropertyAnimation that gives control
    over the direction of rotation during an animation.

    By default, it rotates in the direction
    of the numerical change; a rotation from 0 to 240 will rotate 240 degrees
    clockwise, while a rotation from 240 to 0 will rotate 240 degrees
    counterclockwise. The \l direction property can be set to specify the
    direction in which the rotation should occur.

    In the following example we use RotationAnimation to animate the rotation
    between states via the shortest path:

    \snippet doc/src/snippets/qtquick1/rotationanimation.qml 0

    Notice the RotationAnimation did not need to set a \l target
    value. As a convenience, when used in a transition, RotationAnimation will rotate all
    properties named "rotation" or "angle". You can override this by providing
    your own properties via \l {PropertyAnimation::properties}{properties} or
    \l {PropertyAnimation::property}{property}.

    Also, note the \l Rectangle will be rotated around its default
    \l {Item::}{transformOrigin} (which is \c Item.Center). To use a different
    transform origin, set the origin in the PropertyChanges object and apply
    the change at the start of the animation using PropertyAction. See the
    PropertyAction documentation for more details.

    Like any other animation element, a RotationAnimation can be applied in a
    number of ways, including transitions, behaviors and property value
    sources. The \l {QML Animation and Transitions} documentation shows a
    variety of methods for creating animations.

    \sa {QML Animation and Transitions}, {declarative/animation/basics}{Animation basics example}
*/
QVariant _q_interpolateShortestRotation(qreal &f, qreal &t, qreal progress)
{
    qreal newt = t;
    qreal diff = t-f;
    while(diff > 180.0){
        newt -= 360.0;
        diff -= 360.0;
    }
    while(diff < -180.0){
        newt += 360.0;
        diff += 360.0;
    }
    return QVariant(f + (newt - f) * progress);
}

QVariant _q_interpolateClockwiseRotation(qreal &f, qreal &t, qreal progress)
{
    qreal newt = t;
    qreal diff = t-f;
    while(diff < 0.0){
        newt += 360.0;
        diff += 360.0;
    }
    return QVariant(f + (newt - f) * progress);
}

QVariant _q_interpolateCounterclockwiseRotation(qreal &f, qreal &t, qreal progress)
{
    qreal newt = t;
    qreal diff = t-f;
    while(diff > 0.0){
        newt -= 360.0;
        diff -= 360.0;
    }
    return QVariant(f + (newt - f) * progress);
}

QDeclarative1RotationAnimation::QDeclarative1RotationAnimation(QObject *parent)
: QDeclarative1PropertyAnimation(*(new QDeclarative1RotationAnimationPrivate), parent)
{
    Q_D(QDeclarative1RotationAnimation);
    d->interpolatorType = QMetaType::QReal;
    d->interpolator = QVariantAnimationPrivate::getInterpolator(d->interpolatorType);
    d->defaultProperties = QLatin1String("rotation,angle");
}

QDeclarative1RotationAnimation::~QDeclarative1RotationAnimation()
{
}

/*!
    \qmlproperty real QtQuick1::RotationAnimation::from
    This property holds the starting value for the animation.

    For example, the following animation is not applied until the \c angle value
    has reached 100:

    \qml
    Item {
        states: [
            // ...
        ]

        transition: Transition {
            RotationAnimation { properties: "angle"; from: 100; duration: 2000 }
        }
    }
    \endqml

    If the RotationAnimation is defined within a \l Transition or \l Behavior,
    this value defaults to the value defined in the starting state of the
    \l Transition, or the current value of the property at the moment the
    \l Behavior is triggered.

    \sa {QML Animation and Transitions}
*/
qreal QDeclarative1RotationAnimation::from() const
{
    Q_D(const QDeclarative1RotationAnimation);
    return d->from.toReal();
}

void QDeclarative1RotationAnimation::setFrom(qreal f)
{
    QDeclarative1PropertyAnimation::setFrom(f);
}

/*!
    \qmlproperty real QtQuick1::RotationAnimation::to
    This property holds the end value for the animation..

    If the RotationAnimation is defined within a \l Transition or \l Behavior,
    this value defaults to the value defined in the end state of the
    \l Transition, or the value of the property change that triggered the
    \l Behavior.

    \sa {QML Animation and Transitions}
*/
qreal QDeclarative1RotationAnimation::to() const
{
    Q_D(const QDeclarative1RotationAnimation);
    return d->to.toReal();
}

void QDeclarative1RotationAnimation::setTo(qreal t)
{
    QDeclarative1PropertyAnimation::setTo(t);
}

/*!
    \qmlproperty enumeration QtQuick1::RotationAnimation::direction
    This property holds the direction of the rotation.

    Possible values are:

    \list
    \o RotationAnimation.Numerical (default) - Rotate by linearly interpolating between the two numbers.
           A rotation from 10 to 350 will rotate 340 degrees clockwise.
    \o RotationAnimation.Clockwise - Rotate clockwise between the two values
    \o RotationAnimation.Counterclockwise - Rotate counterclockwise between the two values
    \o RotationAnimation.Shortest - Rotate in the direction that produces the shortest animation path.
           A rotation from 10 to 350 will rotate 20 degrees counterclockwise.
    \endlist
*/
QDeclarative1RotationAnimation::RotationDirection QDeclarative1RotationAnimation::direction() const
{
    Q_D(const QDeclarative1RotationAnimation);
    return d->direction;
}

void QDeclarative1RotationAnimation::setDirection(QDeclarative1RotationAnimation::RotationDirection direction)
{
    Q_D(QDeclarative1RotationAnimation);
    if (d->direction == direction)
        return;

    d->direction = direction;
    switch(d->direction) {
    case Clockwise:
        d->interpolator = reinterpret_cast<QVariantAnimation::Interpolator>(&_q_interpolateClockwiseRotation);
        break;
    case Counterclockwise:
        d->interpolator = reinterpret_cast<QVariantAnimation::Interpolator>(&_q_interpolateCounterclockwiseRotation);
        break;
    case Shortest:
        d->interpolator = reinterpret_cast<QVariantAnimation::Interpolator>(&_q_interpolateShortestRotation);
        break;
    default:
        d->interpolator = QVariantAnimationPrivate::getInterpolator(d->interpolatorType);
        break;
    }

    emit directionChanged();
}



QDeclarative1AnimationGroup::QDeclarative1AnimationGroup(QObject *parent)
: QDeclarative1AbstractAnimation(*(new QDeclarative1AnimationGroupPrivate), parent)
{
}

QDeclarative1AnimationGroup::QDeclarative1AnimationGroup(QDeclarative1AnimationGroupPrivate &dd, QObject *parent)
    : QDeclarative1AbstractAnimation(dd, parent)
{
}

void QDeclarative1AnimationGroupPrivate::append_animation(QDeclarativeListProperty<QDeclarative1AbstractAnimation> *list, QDeclarative1AbstractAnimation *a)
{
    QDeclarative1AnimationGroup *q = qobject_cast<QDeclarative1AnimationGroup *>(list->object);
    if (q) {
        a->setGroup(q);
        // This is an optimization for the parenting that already occurs via addAnimation
        QDeclarative_setParent_noEvent(a->qtAnimation(), q->d_func()->ag);
        q->d_func()->ag->addAnimation(a->qtAnimation());
    }
}

void QDeclarative1AnimationGroupPrivate::clear_animation(QDeclarativeListProperty<QDeclarative1AbstractAnimation> *list)
{
    QDeclarative1AnimationGroup *q = qobject_cast<QDeclarative1AnimationGroup *>(list->object);
    if (q) {
        while (q->d_func()->animations.count()) {
            QDeclarative1AbstractAnimation *firstAnim = q->d_func()->animations.at(0);
            QDeclarative_setParent_noEvent(firstAnim->qtAnimation(), 0);
            q->d_func()->ag->removeAnimation(firstAnim->qtAnimation());
            firstAnim->setGroup(0);
        }
    }
}

QDeclarative1AnimationGroup::~QDeclarative1AnimationGroup()
{
}

QDeclarativeListProperty<QDeclarative1AbstractAnimation> QDeclarative1AnimationGroup::animations()
{
    Q_D(QDeclarative1AnimationGroup);
    QDeclarativeListProperty<QDeclarative1AbstractAnimation> list(this, d->animations);
    list.append = &QDeclarative1AnimationGroupPrivate::append_animation;
    list.clear = &QDeclarative1AnimationGroupPrivate::clear_animation;
    return list;
}

/*!
    \qmlclass SequentialAnimation QDeclarative1SequentialAnimation
    \inqmlmodule QtQuick 1
  \ingroup qml-animation-transition
    \since QtQuick 1.0
    \inherits Animation
    \brief The SequentialAnimation element allows animations to be run sequentially.

    The SequentialAnimation and ParallelAnimation elements allow multiple
    animations to be run together. Animations defined in a SequentialAnimation
    are run one after the other, while animations defined in a ParallelAnimation
    are run at the same time.

    The following example runs two number animations in a sequence.  The \l Rectangle
    animates to a \c x position of 50, then to a \c y position of 50.

    \snippet doc/src/snippets/qtquick1/sequentialanimation.qml 0

    Animations defined within a \l Transition are automatically run in parallel,
    so SequentialAnimation can be used to enclose the animations in a \l Transition
    if this is the preferred behavior.

    Like any other animation element, a SequentialAnimation can be applied in a
    number of ways, including transitions, behaviors and property value
    sources. The \l {QML Animation and Transitions} documentation shows a
    variety of methods for creating animations.

    \note Once an animation has been grouped into a SequentialAnimation or
    ParallelAnimation, it cannot be individually started and stopped; the
    SequentialAnimation or ParallelAnimation must be started and stopped as a group.

    \sa ParallelAnimation, {QML Animation and Transitions}, {declarative/animation/basics}{Animation basics example}
*/

QDeclarative1SequentialAnimation::QDeclarative1SequentialAnimation(QObject *parent) :
    QDeclarative1AnimationGroup(parent)
{
    Q_D(QDeclarative1AnimationGroup);
    d->ag = new QSequentialAnimationGroup;
    QDeclarative_setParent_noEvent(d->ag, this);
}

QDeclarative1SequentialAnimation::~QDeclarative1SequentialAnimation()
{
}

QAbstractAnimation *QDeclarative1SequentialAnimation::qtAnimation()
{
    Q_D(QDeclarative1AnimationGroup);
    return d->ag;
}

void QDeclarative1SequentialAnimation::transition(QDeclarative1StateActions &actions,
                                    QDeclarativeProperties &modified,
                                    TransitionDirection direction)
{
    Q_D(QDeclarative1AnimationGroup);

    int inc = 1;
    int from = 0;
    if (direction == Backward) {
        inc = -1;
        from = d->animations.count() - 1;
    }

    bool valid = d->defaultProperty.isValid();
    for (int ii = from; ii < d->animations.count() && ii >= 0; ii += inc) {
        if (valid)
            d->animations.at(ii)->setDefaultTarget(d->defaultProperty);
        d->animations.at(ii)->transition(actions, modified, direction);
    }
}



/*!
    \qmlclass ParallelAnimation QDeclarative1ParallelAnimation
    \inqmlmodule QtQuick 1
  \ingroup qml-animation-transition
    \since QtQuick 1.0
    \inherits Animation
    \brief The ParallelAnimation element allows animations to be run in parallel.

    The SequentialAnimation and ParallelAnimation elements allow multiple
    animations to be run together. Animations defined in a SequentialAnimation
    are run one after the other, while animations defined in a ParallelAnimation
    are run at the same time.

    The following animation runs two number animations in parallel. The \l Rectangle
    moves to (50,50) by animating its \c x and \c y properties at the same time.

    \snippet doc/src/snippets/qtquick1/parallelanimation.qml 0

    Like any other animation element, a ParallelAnimation can be applied in a
    number of ways, including transitions, behaviors and property value
    sources. The \l {QML Animation and Transitions} documentation shows a
    variety of methods for creating animations.

    \note Once an animation has been grouped into a SequentialAnimation or
    ParallelAnimation, it cannot be individually started and stopped; the
    SequentialAnimation or ParallelAnimation must be started and stopped as a group.

    \sa SequentialAnimation, {QML Animation and Transitions}, {declarative/animation/basics}{Animation basics example}
*/
QDeclarative1ParallelAnimation::QDeclarative1ParallelAnimation(QObject *parent) :
    QDeclarative1AnimationGroup(parent)
{
    Q_D(QDeclarative1AnimationGroup);
    d->ag = new QParallelAnimationGroup;
    QDeclarative_setParent_noEvent(d->ag, this);
}

QDeclarative1ParallelAnimation::~QDeclarative1ParallelAnimation()
{
}

QAbstractAnimation *QDeclarative1ParallelAnimation::qtAnimation()
{
    Q_D(QDeclarative1AnimationGroup);
    return d->ag;
}

void QDeclarative1ParallelAnimation::transition(QDeclarative1StateActions &actions,
                                      QDeclarativeProperties &modified,
                                      TransitionDirection direction)
{
    Q_D(QDeclarative1AnimationGroup);
    bool valid = d->defaultProperty.isValid();
    for (int ii = 0; ii < d->animations.count(); ++ii) {
        if (valid)
            d->animations.at(ii)->setDefaultTarget(d->defaultProperty);
        d->animations.at(ii)->transition(actions, modified, direction);
    }
}



//convert a variant from string type to another animatable type
void QDeclarative1PropertyAnimationPrivate::convertVariant(QVariant &variant, int type)
{
    if (variant.userType() != QVariant::String) {
        variant.convert((QVariant::Type)type);
        return;
    }

    switch (type) {
    case QVariant::Rect: {
        variant.setValue(QDeclarativeStringConverters::rectFFromString(variant.toString()).toRect());
        break;
    }
    case QVariant::RectF: {
        variant.setValue(QDeclarativeStringConverters::rectFFromString(variant.toString()));
        break;
    }
    case QVariant::Point: {
        variant.setValue(QDeclarativeStringConverters::pointFFromString(variant.toString()).toPoint());
        break;
    }
    case QVariant::PointF: {
        variant.setValue(QDeclarativeStringConverters::pointFFromString(variant.toString()));
        break;
    }
    case QVariant::Size: {
        variant.setValue(QDeclarativeStringConverters::sizeFFromString(variant.toString()).toSize());
        break;
    }
    case QVariant::SizeF: {
        variant.setValue(QDeclarativeStringConverters::sizeFFromString(variant.toString()));
        break;
    }
    case QVariant::Color: {
        variant.setValue(QDeclarativeStringConverters::colorFromString(variant.toString()));
        break;
    }
    case QVariant::Vector3D: {
        variant.setValue(QDeclarativeStringConverters::vector3DFromString(variant.toString()));
        break;
    }
    default:
        if (QDeclarativeValueTypeFactory::isValueType((uint)type)) {
            variant.convert((QVariant::Type)type);
        } else {
            QDeclarativeMetaType::StringConverter converter = QDeclarativeMetaType::customStringConverter(type);
            if (converter)
                variant = converter(variant.toString());
        }
        break;
    }
}

/*!
    \qmlclass PropertyAnimation QDeclarative1PropertyAnimation
    \inqmlmodule QtQuick 1
  \ingroup qml-animation-transition
    \since QtQuick 1.0
    \inherits Animation
    \brief The PropertyAnimation element animates changes in property values.

    PropertyAnimation provides a way to animate changes to a property's value.

    It can be used to define animations in a number of ways:

    \list
    \o In a \l Transition

    For example, to animate any objects that have changed their \c x or \c y properties
    as a result of a state change, using an \c InOutQuad easing curve:

    \snippet doc/src/snippets/qtquick1/propertyanimation.qml transition


    \o In a \l Behavior

    For example, to animate all changes to a rectangle's \c x property:

    \snippet doc/src/snippets/qtquick1/propertyanimation.qml behavior


    \o As a property value source

    For example, to repeatedly animate the rectangle's \c x property:

    \snippet doc/src/snippets/qtquick1/propertyanimation.qml propertyvaluesource


    \o In a signal handler

    For example, to fade out \c theObject when clicked:
    \qml
    MouseArea {
        anchors.fill: theObject
        onClicked: PropertyAnimation { target: theObject; property: "opacity"; to: 0 }
    }
    \endqml

    \o Standalone

    For example, to animate \c rect's \c width property over 500ms, from its current width to 30:

    \snippet doc/src/snippets/qtquick1/propertyanimation.qml standalone

    \endlist

    Depending on how the animation is used, the set of properties normally used will be
    different. For more information see the individual property documentation, as well
    as the \l{QML Animation and Transitions} introduction.

    Note that PropertyAnimation inherits the abstract \l Animation element.
    This includes additional properties and methods for controlling the animation.

    \sa {QML Animation and Transitions}, {declarative/animation/basics}{Animation basics example}
*/

QDeclarative1PropertyAnimation::QDeclarative1PropertyAnimation(QObject *parent)
: QDeclarative1AbstractAnimation(*(new QDeclarative1PropertyAnimationPrivate), parent)
{
    Q_D(QDeclarative1PropertyAnimation);
    d->init();
}

QDeclarative1PropertyAnimation::QDeclarative1PropertyAnimation(QDeclarative1PropertyAnimationPrivate &dd, QObject *parent)
: QDeclarative1AbstractAnimation(dd, parent)
{
    Q_D(QDeclarative1PropertyAnimation);
    d->init();
}

QDeclarative1PropertyAnimation::~QDeclarative1PropertyAnimation()
{
}

void QDeclarative1PropertyAnimationPrivate::init()
{
    Q_Q(QDeclarative1PropertyAnimation);
    va = new QDeclarative1BulkValueAnimator;
    QDeclarative_setParent_noEvent(va, q);
}

/*!
    \qmlproperty int QtQuick1::PropertyAnimation::duration
    This property holds the duration of the animation, in milliseconds.

    The default value is 250.
*/
int QDeclarative1PropertyAnimation::duration() const
{
    Q_D(const QDeclarative1PropertyAnimation);
    return d->va->duration();
}

void QDeclarative1PropertyAnimation::setDuration(int duration)
{
    if (duration < 0) {
        qmlInfo(this) << tr("Cannot set a duration of < 0");
        return;
    }

    Q_D(QDeclarative1PropertyAnimation);
    if (d->va->duration() == duration)
        return;
    d->va->setDuration(duration);
    emit durationChanged(duration);
}

/*!
    \qmlproperty real QtQuick1::PropertyAnimation::from
    This property holds the starting value for the animation.

    If the PropertyAnimation is defined within a \l Transition or \l Behavior,
    this value defaults to the value defined in the starting state of the
    \l Transition, or the current value of the property at the moment the
    \l Behavior is triggered.

    \sa {QML Animation and Transitions}
*/
QVariant QDeclarative1PropertyAnimation::from() const
{
    Q_D(const QDeclarative1PropertyAnimation);
    return d->from;
}

void QDeclarative1PropertyAnimation::setFrom(const QVariant &f)
{
    Q_D(QDeclarative1PropertyAnimation);
    if (d->fromIsDefined && f == d->from)
        return;
    d->from = f;
    d->fromIsDefined = f.isValid();
    emit fromChanged(f);
}

/*!
    \qmlproperty real QtQuick1::PropertyAnimation::to
    This property holds the end value for the animation.

    If the PropertyAnimation is defined within a \l Transition or \l Behavior,
    this value defaults to the value defined in the end state of the
    \l Transition, or the value of the property change that triggered the
    \l Behavior.

    \sa {QML Animation and Transitions}
*/
QVariant QDeclarative1PropertyAnimation::to() const
{
    Q_D(const QDeclarative1PropertyAnimation);
    return d->to;
}

void QDeclarative1PropertyAnimation::setTo(const QVariant &t)
{
    Q_D(QDeclarative1PropertyAnimation);
    if (d->toIsDefined && t == d->to)
        return;
    d->to = t;
    d->toIsDefined = t.isValid();
    emit toChanged(t);
}

/*!
    \qmlproperty enumeration QtQuick1::PropertyAnimation::easing.type
    \qmlproperty real QtQuick1::PropertyAnimation::easing.amplitude
    \qmlproperty real QtQuick1::PropertyAnimation::easing.overshoot
    \qmlproperty real QtQuick1::PropertyAnimation::easing.period
    \brief the easing curve used for the animation.

    To specify an easing curve you need to specify at least the type. For some curves you can also specify
    amplitude, period and/or overshoot (more details provided after the table). The default easing curve is
    \c Easing.Linear.

    \qml
    PropertyAnimation { properties: "y"; easing.type: Easing.InOutElastic; easing.amplitude: 2.0; easing.period: 1.5 }
    \endqml

    Available types are:

    \table
    \row
        \o \c Easing.Linear
        \o Easing curve for a linear (t) function: velocity is constant.
        \o \inlineimage qeasingcurve-linear.png
    \row
        \o \c Easing.InQuad
        \o Easing curve for a quadratic (t^2) function: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-inquad.png
    \row
        \o \c Easing.OutQuad
        \o Easing curve for a quadratic (t^2) function: decelerating to zero velocity.
        \o \inlineimage qeasingcurve-outquad.png
    \row
        \o \c Easing.InOutQuad
        \o Easing curve for a quadratic (t^2) function: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutquad.png
    \row
        \o \c Easing.OutInQuad
        \o Easing curve for a quadratic (t^2) function: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outinquad.png
    \row
        \o \c Easing.InCubic
        \o Easing curve for a cubic (t^3) function: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-incubic.png
    \row
        \o \c Easing.OutCubic
        \o Easing curve for a cubic (t^3) function: decelerating from zero velocity.
        \o \inlineimage qeasingcurve-outcubic.png
    \row
        \o \c Easing.InOutCubic
        \o Easing curve for a cubic (t^3) function: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutcubic.png
    \row
        \o \c Easing.OutInCubic
        \o Easing curve for a cubic (t^3) function: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outincubic.png
    \row
        \o \c Easing.InQuart
        \o Easing curve for a quartic (t^4) function: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-inquart.png
    \row
        \o \c Easing.OutQuart
        \o Easing curve for a quartic (t^4) function: decelerating from zero velocity.
        \o \inlineimage qeasingcurve-outquart.png
    \row
        \o \c Easing.InOutQuart
        \o Easing curve for a quartic (t^4) function: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutquart.png
    \row
        \o \c Easing.OutInQuart
        \o Easing curve for a quartic (t^4) function: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outinquart.png
    \row
        \o \c Easing.InQuint
        \o Easing curve for a quintic (t^5) function: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-inquint.png
    \row
        \o \c Easing.OutQuint
        \o Easing curve for a quintic (t^5) function: decelerating from zero velocity.
        \o \inlineimage qeasingcurve-outquint.png
    \row
        \o \c Easing.InOutQuint
        \o Easing curve for a quintic (t^5) function: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutquint.png
    \row
        \o \c Easing.OutInQuint
        \o Easing curve for a quintic (t^5) function: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outinquint.png
    \row
        \o \c Easing.InSine
        \o Easing curve for a sinusoidal (sin(t)) function: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-insine.png
    \row
        \o \c Easing.OutSine
        \o Easing curve for a sinusoidal (sin(t)) function: decelerating from zero velocity.
        \o \inlineimage qeasingcurve-outsine.png
    \row
        \o \c Easing.InOutSine
        \o Easing curve for a sinusoidal (sin(t)) function: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutsine.png
    \row
        \o \c Easing.OutInSine
        \o Easing curve for a sinusoidal (sin(t)) function: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outinsine.png
    \row
        \o \c Easing.InExpo
        \o Easing curve for an exponential (2^t) function: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-inexpo.png
    \row
        \o \c Easing.OutExpo
        \o Easing curve for an exponential (2^t) function: decelerating from zero velocity.
        \o \inlineimage qeasingcurve-outexpo.png
    \row
        \o \c Easing.InOutExpo
        \o Easing curve for an exponential (2^t) function: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutexpo.png
    \row
        \o \c Easing.OutInExpo
        \o Easing curve for an exponential (2^t) function: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outinexpo.png
    \row
        \o \c Easing.InCirc
        \o Easing curve for a circular (sqrt(1-t^2)) function: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-incirc.png
    \row
        \o \c Easing.OutCirc
        \o Easing curve for a circular (sqrt(1-t^2)) function: decelerating from zero velocity.
        \o \inlineimage qeasingcurve-outcirc.png
    \row
        \o \c Easing.InOutCirc
        \o Easing curve for a circular (sqrt(1-t^2)) function: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutcirc.png
    \row
        \o \c Easing.OutInCirc
        \o Easing curve for a circular (sqrt(1-t^2)) function: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outincirc.png
    \row
        \o \c Easing.InElastic
        \o Easing curve for an elastic (exponentially decaying sine wave) function: accelerating from zero velocity.
        \br The peak amplitude can be set with the \e amplitude parameter, and the period of decay by the \e period parameter.
        \o \inlineimage qeasingcurve-inelastic.png
    \row
        \o \c Easing.OutElastic
        \o Easing curve for an elastic (exponentially decaying sine wave) function: decelerating from zero velocity.
        \br The peak amplitude can be set with the \e amplitude parameter, and the period of decay by the \e period parameter.
        \o \inlineimage qeasingcurve-outelastic.png
    \row
        \o \c Easing.InOutElastic
        \o Easing curve for an elastic (exponentially decaying sine wave) function: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutelastic.png
    \row
        \o \c Easing.OutInElastic
        \o Easing curve for an elastic (exponentially decaying sine wave) function: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outinelastic.png
    \row
        \o \c Easing.InBack
        \o Easing curve for a back (overshooting cubic function: (s+1)*t^3 - s*t^2) easing in: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-inback.png
    \row
        \o \c Easing.OutBack
        \o Easing curve for a back (overshooting cubic function: (s+1)*t^3 - s*t^2) easing out: decelerating to zero velocity.
        \o \inlineimage qeasingcurve-outback.png
    \row
        \o \c Easing.InOutBack
        \o Easing curve for a back (overshooting cubic function: (s+1)*t^3 - s*t^2) easing in/out: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutback.png
    \row
        \o \c Easing.OutInBack
        \o Easing curve for a back (overshooting cubic easing: (s+1)*t^3 - s*t^2) easing out/in: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outinback.png
    \row
        \o \c Easing.InBounce
        \o Easing curve for a bounce (exponentially decaying parabolic bounce) function: accelerating from zero velocity.
        \o \inlineimage qeasingcurve-inbounce.png
    \row
        \o \c Easing.OutBounce
        \o Easing curve for a bounce (exponentially decaying parabolic bounce) function: decelerating from zero velocity.
        \o \inlineimage qeasingcurve-outbounce.png
    \row
        \o \c Easing.InOutBounce
        \o Easing curve for a bounce (exponentially decaying parabolic bounce) function easing in/out: acceleration until halfway, then deceleration.
        \o \inlineimage qeasingcurve-inoutbounce.png
    \row
        \o \c Easing.OutInBounce
        \o Easing curve for a bounce (exponentially decaying parabolic bounce) function easing out/in: deceleration until halfway, then acceleration.
        \o \inlineimage qeasingcurve-outinbounce.png
    \endtable

    \c easing.amplitude is only applicable for bounce and elastic curves (curves of type
    \c Easing.InBounce, \c Easing.OutBounce, \c Easing.InOutBounce, \c Easing.OutInBounce, \c Easing.InElastic,
    \c Easing.OutElastic, \c Easing.InOutElastic or \c Easing.OutInElastic).

    \c easing.overshoot is only applicable if \c easing.type is: \c Easing.InBack, \c Easing.OutBack,
    \c Easing.InOutBack or \c Easing.OutInBack.

    \c easing.period is only applicable if easing.type is: \c Easing.InElastic, \c Easing.OutElastic,
    \c Easing.InOutElastic or \c Easing.OutInElastic.

    See the \l {declarative/animation/easing}{easing} example for a demonstration of
    the different easing settings.
*/
QEasingCurve QDeclarative1PropertyAnimation::easing() const
{
    Q_D(const QDeclarative1PropertyAnimation);
    return d->va->easingCurve();
}

void QDeclarative1PropertyAnimation::setEasing(const QEasingCurve &e)
{
    Q_D(QDeclarative1PropertyAnimation);
    if (d->va->easingCurve() == e)
        return;

    d->va->setEasingCurve(e);
    emit easingChanged(e);
}

QObject *QDeclarative1PropertyAnimation::target() const
{
    Q_D(const QDeclarative1PropertyAnimation);
    return d->target;
}

void QDeclarative1PropertyAnimation::setTarget(QObject *o)
{
    Q_D(QDeclarative1PropertyAnimation);
    if (d->target == o)
        return;
    d->target = o;
    emit targetChanged();
}

QString QDeclarative1PropertyAnimation::property() const
{
    Q_D(const QDeclarative1PropertyAnimation);
    return d->propertyName;
}

void QDeclarative1PropertyAnimation::setProperty(const QString &n)
{
    Q_D(QDeclarative1PropertyAnimation);
    if (d->propertyName == n)
        return;
    d->propertyName = n;
    emit propertyChanged();
}

QString QDeclarative1PropertyAnimation::properties() const
{
    Q_D(const QDeclarative1PropertyAnimation);
    return d->properties;
}

void QDeclarative1PropertyAnimation::setProperties(const QString &prop)
{
    Q_D(QDeclarative1PropertyAnimation);
    if (d->properties == prop)
        return;

    d->properties = prop;
    emit propertiesChanged(prop);
}

/*!
    \qmlproperty string QtQuick1::PropertyAnimation::properties
    \qmlproperty list<Object> QtQuick1::PropertyAnimation::targets
    \qmlproperty string QtQuick1::PropertyAnimation::property
    \qmlproperty Object QtQuick1::PropertyAnimation::target

    These properties are used as a set to determine which properties should be animated.
    The singular and plural forms are functionally identical, e.g.
    \qml
    NumberAnimation { target: theItem; property: "x"; to: 500 }
    \endqml
    has the same meaning as
    \qml
    NumberAnimation { targets: theItem; properties: "x"; to: 500 }
    \endqml
    The singular forms are slightly optimized, so if you do have only a single target/property
    to animate you should try to use them.

    The \c targets property allows multiple targets to be set. For example, this animates the
    \c x property of both \c itemA and \c itemB:

    \qml
    NumberAnimation { targets: [itemA, itemB]; properties: "x"; to: 500 }
    \endqml

    In many cases these properties do not need to be explicitly specified, as they can be
    inferred from the animation framework:

    \table 80%
    \row
    \o Value Source / Behavior
    \o When an animation is used as a value source or in a Behavior, the default target and property
       name to be animated can both be inferred.
       \qml
       Rectangle {
           id: theRect
           width: 100; height: 100
           color: Qt.rgba(0,0,1)
           NumberAnimation on x { to: 500; loops: Animation.Infinite } //animate theRect's x property
           Behavior on y { NumberAnimation {} } //animate theRect's y property
       }
       \endqml
    \row
    \o Transition
    \o When used in a transition, a property animation is assumed to match \e all targets
       but \e no properties. In practice, that means you need to specify at least the properties
       in order for the animation to do anything.
       \qml
       Rectangle {
           id: theRect
           width: 100; height: 100
           color: Qt.rgba(0,0,1)
           Item { id: uselessItem }
           states: State {
               name: "state1"
               PropertyChanges { target: theRect; x: 200; y: 200; z: 4 }
               PropertyChanges { target: uselessItem; x: 10; y: 10; z: 2 }
           }
           transitions: Transition {
               //animate both theRect's and uselessItem's x and y to their final values
               NumberAnimation { properties: "x,y" }

               //animate theRect's z to its final value
               NumberAnimation { target: theRect; property: "z" }
           }
       }
       \endqml
    \row
    \o Standalone
    \o When an animation is used standalone, both the target and property need to be
       explicitly specified.
       \qml
       Rectangle {
           id: theRect
           width: 100; height: 100
           color: Qt.rgba(0,0,1)
           //need to explicitly specify target and property
           NumberAnimation { id: theAnim; target: theRect; property: "x"; to: 500 }
           MouseArea {
               anchors.fill: parent
               onClicked: theAnim.start()
           }
       }
       \endqml
    \endtable

    As seen in the above example, properties is specified as a comma-separated string of property names to animate.

    \sa exclude, {QML Animation and Transitions}
*/
QDeclarativeListProperty<QObject> QDeclarative1PropertyAnimation::targets()
{
    Q_D(QDeclarative1PropertyAnimation);
    return QDeclarativeListProperty<QObject>(this, d->targets);
}

/*!
    \qmlproperty list<Object> QtQuick1::PropertyAnimation::exclude
    This property holds the items not to be affected by this animation.
    \sa PropertyAnimation::targets
*/
QDeclarativeListProperty<QObject> QDeclarative1PropertyAnimation::exclude()
{
    Q_D(QDeclarative1PropertyAnimation);
    return QDeclarativeListProperty<QObject>(this, d->exclude);
}

QAbstractAnimation *QDeclarative1PropertyAnimation::qtAnimation()
{
    Q_D(QDeclarative1PropertyAnimation);
    return d->va;
}

void QDeclarative1AnimationPropertyUpdater::setValue(qreal v)
{
    bool deleted = false;
    wasDeleted = &deleted;
    if (reverse)    //QVariantAnimation sends us 1->0 when reversed, but we are expecting 0->1
        v = 1 - v;
    for (int ii = 0; ii < actions.count(); ++ii) {
        QDeclarative1Action &action = actions[ii];

        if (v == 1.)
            QDeclarativePropertyPrivate::write(action.property, action.toValue, QDeclarativePropertyPrivate::BypassInterceptor | QDeclarativePropertyPrivate::DontRemoveBinding);
        else {
            if (!fromSourced && !fromDefined) {
                action.fromValue = action.property.read();
                if (interpolatorType)
                    QDeclarative1PropertyAnimationPrivate::convertVariant(action.fromValue, interpolatorType);
            }
            if (!interpolatorType) {
                int propType = action.property.propertyType();
                if (!prevInterpolatorType || prevInterpolatorType != propType) {
                    prevInterpolatorType = propType;
                    interpolator = QVariantAnimationPrivate::getInterpolator(prevInterpolatorType);
                }
            }
            if (interpolator)
                QDeclarativePropertyPrivate::write(action.property, interpolator(action.fromValue.constData(), action.toValue.constData(), v), QDeclarativePropertyPrivate::BypassInterceptor | QDeclarativePropertyPrivate::DontRemoveBinding);
        }
        if (deleted)
            return;
    }
    wasDeleted = 0;
    fromSourced = true;
}

void QDeclarative1PropertyAnimation::transition(QDeclarative1StateActions &actions,
                                     QDeclarativeProperties &modified,
                                     TransitionDirection direction)
{
    Q_D(QDeclarative1PropertyAnimation);

    QStringList props = d->properties.isEmpty() ? QStringList() : d->properties.split(QLatin1Char(','));
    for (int ii = 0; ii < props.count(); ++ii)
        props[ii] = props.at(ii).trimmed();
    if (!d->propertyName.isEmpty())
        props << d->propertyName;

    QList<QObject*> targets = d->targets;
    if (d->target)
        targets.append(d->target);

    bool hasSelectors = !props.isEmpty() || !targets.isEmpty() || !d->exclude.isEmpty();
    bool useType = (props.isEmpty() && d->defaultToInterpolatorType) ? true : false;

    if (d->defaultProperty.isValid() && !hasSelectors) {
        props << d->defaultProperty.name();
        targets << d->defaultProperty.object();
    }

    if (props.isEmpty() && !d->defaultProperties.isEmpty()) {
        props << d->defaultProperties.split(QLatin1Char(','));
    }

    QDeclarative1AnimationPropertyUpdater *data = new QDeclarative1AnimationPropertyUpdater;
    data->interpolatorType = d->interpolatorType;
    data->interpolator = d->interpolator;
    data->reverse = direction == Backward ? true : false;
    data->fromSourced = false;
    data->fromDefined = d->fromIsDefined;

    bool hasExplicit = false;
    //an explicit animation has been specified
    if (d->toIsDefined) {
        for (int i = 0; i < props.count(); ++i) {
            for (int j = 0; j < targets.count(); ++j) {
                QDeclarative1Action myAction;
                myAction.property = d->createProperty(targets.at(j), props.at(i), this);
                if (myAction.property.isValid()) {
                    if (d->fromIsDefined) {
                        myAction.fromValue = d->from;
                        d->convertVariant(myAction.fromValue, d->interpolatorType ? d->interpolatorType : myAction.property.propertyType());
                    }
                    myAction.toValue = d->to;
                    d->convertVariant(myAction.toValue, d->interpolatorType ? d->interpolatorType : myAction.property.propertyType());
                    data->actions << myAction;
                    hasExplicit = true;
                    for (int ii = 0; ii < actions.count(); ++ii) {
                        QDeclarative1Action &action = actions[ii];
                        if (action.property.object() == myAction.property.object() &&
                            myAction.property.name() == action.property.name()) {
                            modified << action.property;
                            break;  //### any chance there could be multiples?
                        }
                    }
                }
            }
        }
    }

    if (!hasExplicit)
    for (int ii = 0; ii < actions.count(); ++ii) {
        QDeclarative1Action &action = actions[ii];

        QObject *obj = action.property.object();
        QString propertyName = action.property.name();
        QObject *sObj = action.specifiedObject;
        QString sPropertyName = action.specifiedProperty;
        bool same = (obj == sObj);

        if ((targets.isEmpty() || targets.contains(obj) || (!same && targets.contains(sObj))) &&
           (!d->exclude.contains(obj)) && (same || (!d->exclude.contains(sObj))) &&
           (props.contains(propertyName) || (!same && props.contains(sPropertyName))
               || (useType && action.property.propertyType() == d->interpolatorType))) {
            QDeclarative1Action myAction = action;

            if (d->fromIsDefined)
                myAction.fromValue = d->from;
            else
                myAction.fromValue = QVariant();
            if (d->toIsDefined)
                myAction.toValue = d->to;

            d->convertVariant(myAction.fromValue, d->interpolatorType ? d->interpolatorType : myAction.property.propertyType());
            d->convertVariant(myAction.toValue, d->interpolatorType ? d->interpolatorType : myAction.property.propertyType());

            modified << action.property;

            data->actions << myAction;
            action.fromValue = myAction.toValue;
        }
    }

    if (data->actions.count()) {
        if (!d->rangeIsSet) {
            d->va->setStartValue(qreal(0));
            d->va->setEndValue(qreal(1));
            d->rangeIsSet = true;
        }
        d->va->setAnimValue(data, QAbstractAnimation::DeleteWhenStopped);
        d->va->setFromSourcedValue(&data->fromSourced);
        d->actions = &data->actions;
    } else {
        delete data;
        d->va->setFromSourcedValue(0);  //clear previous data
        d->va->setAnimValue(0, QAbstractAnimation::DeleteWhenStopped);  //clear previous data
        d->actions = 0;
    }
}

/*!
    \qmlclass ParentAnimation QDeclarative1ParentAnimation
    \inqmlmodule QtQuick 1
  \ingroup qml-animation-transition
    \since QtQuick 1.0
    \inherits Animation
    \brief The ParentAnimation element animates changes in parent values.

    ParentAnimation is used to animate a parent change for an \l Item.

    For example, the following ParentChange changes \c blueRect to become
    a child of \c redRect when it is clicked. The inclusion of the
    ParentAnimation, which defines a NumberAnimation to be applied during
    the transition, ensures the item animates smoothly as it moves to
    its new parent:

    \snippet doc/src/snippets/qtquick1/parentanimation.qml 0

    A ParentAnimation can contain any number of animations. These animations will
    be run in parallel; to run them sequentially, define them within a
    SequentialAnimation.

    In some cases, such as when reparenting between items with clipping enabled, it is useful
    to animate the parent change via another item that does not have clipping
    enabled. Such an item can be set using the \l via property.

    For convenience, when a ParentAnimation is used in a \l Transition, it will
    animate any ParentChange that has occurred during the state change.
    This can be overridden by setting a specific target item using the
    \l target property.

    Like any other animation element, a ParentAnimation can be applied in a
    number of ways, including transitions, behaviors and property value
    sources. The \l {QML Animation and Transitions} documentation shows a
    variety of methods for creating animations.

    \sa {QML Animation and Transitions}, {declarative/animation/basics}{Animation basics example}
*/
QDeclarative1ParentAnimation::QDeclarative1ParentAnimation(QObject *parent)
    : QDeclarative1AnimationGroup(*(new QDeclarative1ParentAnimationPrivate), parent)
{
    Q_D(QDeclarative1ParentAnimation);
    d->topLevelGroup = new QSequentialAnimationGroup;
    QDeclarative_setParent_noEvent(d->topLevelGroup, this);

    d->startAction = new QActionAnimation_1;
    QDeclarative_setParent_noEvent(d->startAction, d->topLevelGroup);
    d->topLevelGroup->addAnimation(d->startAction);

    d->ag = new QParallelAnimationGroup;
    QDeclarative_setParent_noEvent(d->ag, d->topLevelGroup);
    d->topLevelGroup->addAnimation(d->ag);

    d->endAction = new QActionAnimation_1;
    QDeclarative_setParent_noEvent(d->endAction, d->topLevelGroup);
    d->topLevelGroup->addAnimation(d->endAction);
}

QDeclarative1ParentAnimation::~QDeclarative1ParentAnimation()
{
}

/*!
    \qmlproperty Item QtQuick1::ParentAnimation::target
    The item to reparent.

    When used in a transition, if no target is specified, all
    ParentChange occurrences are animated by the ParentAnimation.
*/
QDeclarativeItem *QDeclarative1ParentAnimation::target() const
{
    Q_D(const QDeclarative1ParentAnimation);
    return d->target;
}

void QDeclarative1ParentAnimation::setTarget(QDeclarativeItem *target)
{
    Q_D(QDeclarative1ParentAnimation);
    if (target == d->target)
        return;

    d->target = target;
    emit targetChanged();
}

/*!
    \qmlproperty Item QtQuick1::ParentAnimation::newParent
    The new parent to animate to.

    If the ParentAnimation is defined within a \l Transition or \l Behavior,
    this value defaults to the value defined in the end state of the
    \l Transition, or the value of the property change that triggered the
    \l Behavior.
*/
QDeclarativeItem *QDeclarative1ParentAnimation::newParent() const
{
    Q_D(const QDeclarative1ParentAnimation);
    return d->newParent;
}

void QDeclarative1ParentAnimation::setNewParent(QDeclarativeItem *newParent)
{
    Q_D(QDeclarative1ParentAnimation);
    if (newParent == d->newParent)
        return;

    d->newParent = newParent;
    emit newParentChanged();
}

/*!
    \qmlproperty Item QtQuick1::ParentAnimation::via
    The item to reparent via. This provides a way to do an unclipped animation
    when both the old parent and new parent are clipped.

    \qml
    ParentAnimation {
        target: myItem
        via: topLevelItem
        // ...
    }
    \endqml
*/
QDeclarativeItem *QDeclarative1ParentAnimation::via() const
{
    Q_D(const QDeclarative1ParentAnimation);
    return d->via;
}

void QDeclarative1ParentAnimation::setVia(QDeclarativeItem *via)
{
    Q_D(QDeclarative1ParentAnimation);
    if (via == d->via)
        return;

    d->via = via;
    emit viaChanged();
}

//### mirrors same-named function in QDeclarativeItem
QPointF QDeclarative1ParentAnimationPrivate::computeTransformOrigin(QDeclarativeItem::TransformOrigin origin, qreal width, qreal height) const
{
    switch(origin) {
    default:
    case QDeclarativeItem::TopLeft:
        return QPointF(0, 0);
    case QDeclarativeItem::Top:
        return QPointF(width / 2., 0);
    case QDeclarativeItem::TopRight:
        return QPointF(width, 0);
    case QDeclarativeItem::Left:
        return QPointF(0, height / 2.);
    case QDeclarativeItem::Center:
        return QPointF(width / 2., height / 2.);
    case QDeclarativeItem::Right:
        return QPointF(width, height / 2.);
    case QDeclarativeItem::BottomLeft:
        return QPointF(0, height);
    case QDeclarativeItem::Bottom:
        return QPointF(width / 2., height);
    case QDeclarativeItem::BottomRight:
        return QPointF(width, height);
    }
}

void QDeclarative1ParentAnimation::transition(QDeclarative1StateActions &actions,
                        QDeclarativeProperties &modified,
                        TransitionDirection direction)
{
    Q_D(QDeclarative1ParentAnimation);

    struct QDeclarative1ParentAnimationData : public QAbstractAnimationAction
    {
        QDeclarative1ParentAnimationData() {}
        ~QDeclarative1ParentAnimationData() { qDeleteAll(pc); }

        QDeclarative1StateActions actions;
        //### reverse should probably apply on a per-action basis
        bool reverse;
        QList<QDeclarative1ParentChange *> pc;
        virtual void doAction()
        {
            for (int ii = 0; ii < actions.count(); ++ii) {
                const QDeclarative1Action &action = actions.at(ii);
                if (reverse)
                    action.event->reverse();
                else
                    action.event->execute();
            }
        }
    };

    QDeclarative1ParentAnimationData *data = new QDeclarative1ParentAnimationData;
    QDeclarative1ParentAnimationData *viaData = new QDeclarative1ParentAnimationData;

    bool hasExplicit = false;
    if (d->target && d->newParent) {
        data->reverse = false;
        QDeclarative1Action myAction;
        QDeclarative1ParentChange *pc = new QDeclarative1ParentChange;
        pc->setObject(d->target);
        pc->setParent(d->newParent);
        myAction.event = pc;
        data->pc << pc;
        data->actions << myAction;
        hasExplicit = true;
        if (d->via) {
            viaData->reverse = false;
            QDeclarative1Action myVAction;
            QDeclarative1ParentChange *vpc = new QDeclarative1ParentChange;
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
        QDeclarative1Action &action = actions[i];
        if (action.event && action.event->typeName() == QLatin1String("ParentChange")
            && (!d->target || static_cast<QDeclarative1ParentChange*>(action.event)->object() == d->target)) {

            QDeclarative1ParentChange *pc = static_cast<QDeclarative1ParentChange*>(action.event);
            QDeclarative1Action myAction = action;
            data->reverse = action.reverseEvent;

            //### this logic differs from PropertyAnimation
            //    (probably a result of modified vs. done)
            if (d->newParent) {
                QDeclarative1ParentChange *epc = new QDeclarative1ParentChange;
                epc->setObject(static_cast<QDeclarative1ParentChange*>(action.event)->object());
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
                QDeclarative1Action myAction;
                QDeclarative1ParentChange *vpc = new QDeclarative1ParentChange;
                vpc->setObject(pc->object());
                vpc->setParent(d->via);
                myAction.event = vpc;
                viaData->pc << vpc;
                viaData->actions << myAction;
                QDeclarative1Action dummyAction;
                QDeclarative1Action &xAction = pc->xIsSet() && i < actions.size()-1 ? actions[++i] : dummyAction;
                QDeclarative1Action &yAction = pc->yIsSet() && i < actions.size()-1 ? actions[++i] : dummyAction;
                QDeclarative1Action &sAction = pc->scaleIsSet() && i < actions.size()-1 ? actions[++i] : dummyAction;
                QDeclarative1Action &rAction = pc->rotationIsSet() && i < actions.size()-1 ? actions[++i] : dummyAction;
                QDeclarativeItem *target = pc->object();
                QDeclarativeItem *targetParent = action.reverseEvent ? pc->originalParent() : pc->parent();

                //### this mirrors the logic in QDeclarative1ParentChange.
                bool ok;
                const QTransform &transform = targetParent->itemTransform(d->via, &ok);
                if (transform.type() >= QTransform::TxShear || !ok) {
                    qmlInfo(this) << QDeclarative1ParentAnimation::tr("Unable to preserve appearance under complex transform");
                    ok = false;
                }

                qreal scale = 1;
                qreal rotation = 0;
                bool isRotate = (transform.type() == QTransform::TxRotate) || (transform.m11() < 0);
                if (ok && !isRotate) {
                    if (transform.m11() == transform.m22())
                        scale = transform.m11();
                    else {
                        qmlInfo(this) << QDeclarative1ParentAnimation::tr("Unable to preserve appearance under non-uniform scale");
                        ok = false;
                    }
                } else if (ok && isRotate) {
                    if (transform.m11() == transform.m22())
                        scale = qSqrt(transform.m11()*transform.m11() + transform.m12()*transform.m12());
                    else {
                        qmlInfo(this) << QDeclarative1ParentAnimation::tr("Unable to preserve appearance under non-uniform scale");
                        ok = false;
                    }

                    if (scale != 0)
                        rotation = atan2(transform.m12()/scale, transform.m11()/scale) * 180/M_PI;
                    else {
                        qmlInfo(this) << QDeclarative1ParentAnimation::tr("Unable to preserve appearance under scale of 0");
                        ok = false;
                    }
                }

                const QPointF &point = transform.map(QPointF(xAction.toValue.toReal(),yAction.toValue.toReal()));
                qreal x = point.x();
                qreal y = point.y();
                if (ok && target->transformOrigin() != QDeclarativeItem::TopLeft) {
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

    if (data->actions.count()) {
        if (direction == QDeclarative1AbstractAnimation::Forward) {
            d->startAction->setAnimAction(d->via ? viaData : data, QActionAnimation_1::DeleteWhenStopped);
            d->endAction->setAnimAction(d->via ? data : 0, QActionAnimation_1::DeleteWhenStopped);
        } else {
            d->endAction->setAnimAction(d->via ? viaData : data, QActionAnimation_1::DeleteWhenStopped);
            d->startAction->setAnimAction(d->via ? data : 0, QActionAnimation_1::DeleteWhenStopped);
        }
        if (!d->via)
            delete viaData;
    } else {
        delete data;
        delete viaData;
    }

    //take care of any child animations
    bool valid = d->defaultProperty.isValid();
    for (int ii = 0; ii < d->animations.count(); ++ii) {
        if (valid)
            d->animations.at(ii)->setDefaultTarget(d->defaultProperty);
        d->animations.at(ii)->transition(actions, modified, direction);
    }

}

QAbstractAnimation *QDeclarative1ParentAnimation::qtAnimation()
{
    Q_D(QDeclarative1ParentAnimation);
    return d->topLevelGroup;
}

/*!
    \qmlclass AnchorAnimation QDeclarative1AnchorAnimation
    \inqmlmodule QtQuick 1
  \ingroup qml-animation-transition
    \since QtQuick 1.0
    \inherits Animation
    \brief The AnchorAnimation element animates changes in anchor values.

    AnchorAnimation is used to animate an anchor change.

    In the following snippet we animate the addition of a right anchor to a \l Rectangle:

    \snippet doc/src/snippets/qtquick1/anchoranimation.qml 0

    For convenience, when an AnchorAnimation is used in a \l Transition, it will
    animate any AnchorChanges that have occurred during the state change.
    This can be overridden by setting a specific target item using the
    \l target property.

    Like any other animation element, an AnchorAnimation can be applied in a
    number of ways, including transitions, behaviors and property value
    sources. The \l {QML Animation and Transitions} documentation shows a
    variety of methods for creating animations.

    \sa {QML Animation and Transitions}, AnchorChanges
*/

QDeclarative1AnchorAnimation::QDeclarative1AnchorAnimation(QObject *parent)
: QDeclarative1AbstractAnimation(*(new QDeclarative1AnchorAnimationPrivate), parent)
{
    Q_D(QDeclarative1AnchorAnimation);
    d->va = new QDeclarative1BulkValueAnimator;
    QDeclarative_setParent_noEvent(d->va, this);
}

QDeclarative1AnchorAnimation::~QDeclarative1AnchorAnimation()
{
}

QAbstractAnimation *QDeclarative1AnchorAnimation::qtAnimation()
{
    Q_D(QDeclarative1AnchorAnimation);
    return d->va;
}

/*!
    \qmlproperty list<Item> QtQuick1::AnchorAnimation::targets
    The items to reanchor.

    If no targets are specified all AnchorChanges will be
    animated by the AnchorAnimation.
*/
QDeclarativeListProperty<QDeclarativeItem> QDeclarative1AnchorAnimation::targets()
{
    Q_D(QDeclarative1AnchorAnimation);
    return QDeclarativeListProperty<QDeclarativeItem>(this, d->targets);
}

/*!
    \qmlproperty int QtQuick1::AnchorAnimation::duration
    This property holds the duration of the animation, in milliseconds.

    The default value is 250.
*/
int QDeclarative1AnchorAnimation::duration() const
{
    Q_D(const QDeclarative1AnchorAnimation);
    return d->va->duration();
}

void QDeclarative1AnchorAnimation::setDuration(int duration)
{
    if (duration < 0) {
        qmlInfo(this) << tr("Cannot set a duration of < 0");
        return;
    }

    Q_D(QDeclarative1AnchorAnimation);
    if (d->va->duration() == duration)
        return;
    d->va->setDuration(duration);
    emit durationChanged(duration);
}

/*!
    \qmlproperty enumeration QtQuick1::AnchorAnimation::easing.type
    \qmlproperty real QtQuick1::AnchorAnimation::easing.amplitude
    \qmlproperty real QtQuick1::AnchorAnimation::easing.overshoot
    \qmlproperty real QtQuick1::AnchorAnimation::easing.period
    \brief the easing curve used for the animation.

    To specify an easing curve you need to specify at least the type. For some curves you can also specify
    amplitude, period and/or overshoot. The default easing curve is
    Linear.

    \qml
    AnchorAnimation { easing.type: Easing.InOutQuad }
    \endqml

    See the \l{PropertyAnimation::easing.type} documentation for information
    about the different types of easing curves.
*/

QEasingCurve QDeclarative1AnchorAnimation::easing() const
{
    Q_D(const QDeclarative1AnchorAnimation);
    return d->va->easingCurve();
}

void QDeclarative1AnchorAnimation::setEasing(const QEasingCurve &e)
{
    Q_D(QDeclarative1AnchorAnimation);
    if (d->va->easingCurve() == e)
        return;

    d->va->setEasingCurve(e);
    emit easingChanged(e);
}

void QDeclarative1AnchorAnimation::transition(QDeclarative1StateActions &actions,
                        QDeclarativeProperties &modified,
                        TransitionDirection direction)
{
    Q_UNUSED(modified);
    Q_D(QDeclarative1AnchorAnimation);
    QDeclarative1AnimationPropertyUpdater *data = new QDeclarative1AnimationPropertyUpdater;
    data->interpolatorType = QMetaType::QReal;
    data->interpolator = d->interpolator;

    data->reverse = direction == Backward ? true : false;
    data->fromSourced = false;
    data->fromDefined = false;

    for (int ii = 0; ii < actions.count(); ++ii) {
        QDeclarative1Action &action = actions[ii];
        if (action.event && action.event->typeName() == QLatin1String("AnchorChanges")
            && (d->targets.isEmpty() || d->targets.contains(static_cast<QDeclarative1AnchorChanges*>(action.event)->object()))) {
            data->actions << static_cast<QDeclarative1AnchorChanges*>(action.event)->additionalActions();
        }
    }

    if (data->actions.count()) {
        if (!d->rangeIsSet) {
            d->va->setStartValue(qreal(0));
            d->va->setEndValue(qreal(1));
            d->rangeIsSet = true;
        }
        d->va->setAnimValue(data, QAbstractAnimation::DeleteWhenStopped);
        d->va->setFromSourcedValue(&data->fromSourced);
    } else {
        delete data;
    }
}

QDeclarative1ScriptActionPrivate::QDeclarative1ScriptActionPrivate()
    : QDeclarative1AbstractAnimationPrivate(), hasRunScriptScript(false), reversing(false), proxy(this), rsa(0) {}




QT_END_NAMESPACE
