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

#include "QtQuick1/private/qdeclarativetimer_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qpauseanimation.h>
#include <qdebug.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE





class QDeclarative1TimerPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1Timer)
public:
    QDeclarative1TimerPrivate()
        : interval(1000), running(false), repeating(false), triggeredOnStart(false)
        , classBegun(false), componentComplete(false), firstTick(true) {}
    int interval;
    QPauseAnimation pause;
    bool running : 1;
    bool repeating : 1;
    bool triggeredOnStart : 1;
    bool classBegun : 1;
    bool componentComplete : 1;
    bool firstTick : 1;
};

/*!
    \qmlclass Timer QDeclarative1Timer
    \inqmlmodule QtQuick 1
    \ingroup qml-utility-elements
    \since QtQuick 1.0
    \brief The Timer item triggers a handler at a specified interval.

    A Timer can be used to trigger an action either once, or repeatedly
    at a given interval.

    Here is a Timer that shows the current date and time, and updates
    the text every 500 milliseconds. It uses the JavaScript \c Date
    object to access the current time.

    \qml
    import QtQuick 1.0

    Item {
        Timer {
            interval: 500; running: true; repeat: true
            onTriggered: time.text = Date().toString()
        }

        Text { id: time }
    }
    \endqml

    The Timer element is synchronized with the animation timer.  Since the animation
    timer is usually set to 60fps, the resolution of Timer will be
    at best 16ms.

    If the Timer is running and one of its properties is changed, the
    elapsed time will be reset.  For example, if a Timer with interval of
    1000ms has its \e repeat property changed 500ms after starting, the
    elapsed time will be reset to 0, and the Timer will be triggered
    1000ms later.

    \sa {declarative/toys/clocks}{Clocks example}
*/

QDeclarative1Timer::QDeclarative1Timer(QObject *parent)
    : QObject(*(new QDeclarative1TimerPrivate), parent)
{
    Q_D(QDeclarative1Timer);
    connect(&d->pause, SIGNAL(currentLoopChanged(int)), this, SLOT(ticked()));
    connect(&d->pause, SIGNAL(finished()), this, SLOT(finished()));
    d->pause.setLoopCount(1);
    d->pause.setDuration(d->interval);
}

/*!
    \qmlproperty int QtQuick1::Timer::interval

    Sets the \a interval between triggers, in milliseconds.

    The default interval is 1000 milliseconds.
*/
void QDeclarative1Timer::setInterval(int interval)
{
    Q_D(QDeclarative1Timer);
    if (interval != d->interval) {
        d->interval = interval;
        update();
        emit intervalChanged();
    }
}

int QDeclarative1Timer::interval() const
{
    Q_D(const QDeclarative1Timer);
    return d->interval;
}

/*!
    \qmlproperty bool QtQuick1::Timer::running

    If set to true, starts the timer; otherwise stops the timer.
    For a non-repeating timer, \a running is set to false after the
    timer has been triggered.

    \a running defaults to false.

    \sa repeat
*/
bool QDeclarative1Timer::isRunning() const
{
    Q_D(const QDeclarative1Timer);
    return d->running;
}

void QDeclarative1Timer::setRunning(bool running)
{
    Q_D(QDeclarative1Timer);
    if (d->running != running) {
        d->running = running;
        d->firstTick = true;
        emit runningChanged();
        update();
    }
}

/*!
    \qmlproperty bool QtQuick1::Timer::repeat

    If \a repeat is true the timer is triggered repeatedly at the
    specified interval; otherwise, the timer will trigger once at the
    specified interval and then stop (i.e. running will be set to false).

    \a repeat defaults to false.

    \sa running
*/
bool QDeclarative1Timer::isRepeating() const
{
    Q_D(const QDeclarative1Timer);
    return d->repeating;
}

void QDeclarative1Timer::setRepeating(bool repeating)
{
    Q_D(QDeclarative1Timer);
    if (repeating != d->repeating) {
        d->repeating = repeating;
        update();
        emit repeatChanged();
    }
}

/*!
    \qmlproperty bool QtQuick1::Timer::triggeredOnStart

    When a timer is started, the first trigger is usually after the specified
    interval has elapsed.  It is sometimes desirable to trigger immediately
    when the timer is started; for example, to establish an initial
    state.

    If \a triggeredOnStart is true, the timer is triggered immediately
    when started, and subsequently at the specified interval. Note that if
    \e repeat is set to false, the timer is triggered twice; once on start,
    and again at the interval.

    \a triggeredOnStart defaults to false.

    \sa running
*/
bool QDeclarative1Timer::triggeredOnStart() const
{
    Q_D(const QDeclarative1Timer);
    return d->triggeredOnStart;
}

void QDeclarative1Timer::setTriggeredOnStart(bool triggeredOnStart)
{
    Q_D(QDeclarative1Timer);
    if (d->triggeredOnStart != triggeredOnStart) {
        d->triggeredOnStart = triggeredOnStart;
        update();
        emit triggeredOnStartChanged();
    }
}

/*!
    \qmlmethod QtQuick1::Timer::start()
    \brief Starts the timer.

    If the timer is already running, calling this method has no effect.  The
    \c running property will be true following a call to \c start().
*/
void QDeclarative1Timer::start()
{
    setRunning(true);
}

/*!
    \qmlmethod QtQuick1::Timer::stop()
    \brief Stops the timer.

    If the timer is not running, calling this method has no effect.  The
    \c running property will be false following a call to \c stop().
*/
void QDeclarative1Timer::stop()
{
    setRunning(false);
}

/*!
    \qmlmethod QtQuick1::Timer::restart()
    \brief Restarts the timer.

    If the Timer is not running it will be started, otherwise it will be
    stopped, reset to initial state and started.  The \c running property
    will be true following a call to \c restart().
*/
void QDeclarative1Timer::restart()
{
    setRunning(false);
    setRunning(true);
}

void QDeclarative1Timer::update()
{
    Q_D(QDeclarative1Timer);
    if (d->classBegun && !d->componentComplete)
        return;
    d->pause.stop();
    if (d->running) {
        d->pause.setCurrentTime(0);
        d->pause.setLoopCount(d->repeating ? -1 : 1);
        d->pause.setDuration(d->interval);
        d->pause.start();
        if (d->triggeredOnStart && d->firstTick) {
            QCoreApplication::removePostedEvents(this, QEvent::MetaCall);
            QMetaObject::invokeMethod(this, "ticked", Qt::QueuedConnection);
        }
    }
}

void QDeclarative1Timer::classBegin()
{
    Q_D(QDeclarative1Timer);
    d->classBegun = true;
}

void QDeclarative1Timer::componentComplete()
{
    Q_D(QDeclarative1Timer);
    d->componentComplete = true;
    update();
}

/*!
    \qmlsignal QtQuick1::Timer::onTriggered()

    This handler is called when the Timer is triggered.
*/
void QDeclarative1Timer::ticked()
{
    Q_D(QDeclarative1Timer);
    if (d->running && (d->pause.currentTime() > 0 || (d->triggeredOnStart && d->firstTick)))
        emit triggered();
    d->firstTick = false;
}

void QDeclarative1Timer::finished()
{
    Q_D(QDeclarative1Timer);
    if (d->repeating || !d->running)
        return;
    emit triggered();
    d->running = false;
    d->firstTick = false;
    emit runningChanged();
}



QT_END_NAMESPACE
