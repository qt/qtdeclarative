// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickframeanimation_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qelapsedtimer.h>
#include "private/qabstractanimationjob_p.h"
#include <private/qobject_p.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

class QFrameAnimationJob : public QAbstractAnimationJob
{
    int duration() const override {
        return 1;
    }
};

class QQuickFrameAnimationPrivate : public QObjectPrivate, public QAnimationJobChangeListener
{
    Q_DECLARE_PUBLIC(QQuickFrameAnimation)
public:
    QQuickFrameAnimationPrivate() {}

    void animationCurrentLoopChanged(QAbstractAnimationJob *) override {
        maybeTick();
    }

    void maybeTick()
    {
        Q_Q(QQuickFrameAnimation);
        if (!running || paused)
            return;

        qint64 elapsedTimeNs = elapsedTimer.nsecsElapsed();
        qint64 frameTimeNs = elapsedTimeNs - prevElapsedTimeNs;
        if (prevFrameTimeNs != frameTimeNs) {
            frameTime = qreal(frameTimeNs) / 1000000000.0;
            Q_EMIT q->frameTimeChanged();
        }

        const qreal f = 0.1;
        qreal newSmoothFrameTime = f * frameTime + (1.0 - f) * smoothFrameTime;
        if (!qFuzzyCompare(newSmoothFrameTime, smoothFrameTime)) {
            smoothFrameTime = newSmoothFrameTime;
            Q_EMIT q->smoothFrameTimeChanged();
        }

        q->setElapsedTime(elapsedTime + frameTime);

        const int frame = (firstTick && currentFrame > 0) ? 0 : currentFrame + 1;
        q->setCurrentFrame(frame);

        prevElapsedTimeNs = elapsedTimeNs;
        prevFrameTimeNs = frameTimeNs;
        firstTick = false;

        Q_EMIT q->triggered();
    }

    // Handle the running/pausing state updates.
    void updateState()
    {
        if (!componentComplete)
            return;

        if (running && !paused) {
            if (firstTick) {
                elapsedTime = 0;
                elapsedTimer.start();
            }
            prevElapsedTimeNs = elapsedTimer.nsecsElapsed();
            frameJob.start();
        } else {
            frameJob.stop();
        }
    }

private:
    QFrameAnimationJob frameJob;
    QElapsedTimer elapsedTimer;
    int currentFrame = 0;
    qreal frameTime = 0.0;
    qreal smoothFrameTime = 0.0;
    qreal elapsedTime = 0.0;
    qint64 prevFrameTimeNs = 0;
    qint64 prevElapsedTimeNs = 0;
    bool running = false;
    bool paused = false;
    bool componentComplete = false;
    bool firstTick = true;
};

/*!
    \qmltype FrameAnimation
    \instantiates QQuickFrameAnimation
    \inqmlmodule QtQuick
    \ingroup qtquick-interceptors
    \since 6.4
    \brief Triggers a handler at every animation frame update.

    A FrameAnimation can be used to trigger an action every time animations
    have progressed and an animation frame has been rendered. See the documentation
    about the \l{qtquick-visualcanvas-scenegraph.html}{Scene Graph} for in-depth
    information about the threaded and basic render loops.

    For general animations, prefer using \c NumberAnimation and other \c Animation
    elements as those provide declarative way to describe the animations.

    FrameAnimation on the other hand should be used for custom imperative animations
    and in use-cases like these:
    \list
    \li When you need to run some code on every frame update. Or e.g. every other frame,
        maybe using progressive rendering.
    \li When the speed / target is changing during the animation, normal QML animations
        can be too limiting.
    \li When more accurate frame update time is needed, e.g. for fps counter.
    \endlist

    Compared to \c Timer which allows to set the \c interval time, FrameAnimation runs
    always in synchronization with the animation updates. If you have used \c Timer
    with a short interval for custom animations like below, please consider switching
    to use FrameAnimation instead for smoother animations.
    \code
    // BAD
    Timer {
        interval: 16
        repeat: true
        running: true
        onTriggered: {
            // Animate something
        }
    }

    // GOOD
    FrameAnimation {
        running: true
        onTriggered: {
            // Animate something
        }
    }
    \endcode
*/
QQuickFrameAnimation::QQuickFrameAnimation(QObject *parent)
    : QObject(*(new QQuickFrameAnimationPrivate), parent)
{
    Q_D(QQuickFrameAnimation);
    d->frameJob.addAnimationChangeListener(d, QAbstractAnimationJob::CurrentLoop);
    d->frameJob.setLoopCount(-1);
}

/*!
    \qmlsignal QtQuick::FrameAnimation::triggered()

    This signal is emitted when the FrameAnimation has progressed to a new frame.
*/

/*!
    \qmlproperty bool QtQuick::FrameAnimation::running

    If set to true, starts the frame animation; otherwise stops it.

    \a running defaults to false.

    \sa stop(), start(), restart()
*/
bool QQuickFrameAnimation::isRunning() const
{
    Q_D(const QQuickFrameAnimation);
    return d->running;
}

void QQuickFrameAnimation::setRunning(bool running)
{
    Q_D(QQuickFrameAnimation);
    if (d->running != running) {
        d->running = running;
        d->firstTick = true;
        Q_EMIT runningChanged();
        d->updateState();
    }
}

/*!
    \qmlproperty bool QtQuick::FrameAnimation::paused

    If set to true, pauses the frame animation; otherwise resumes it.

    \a paused defaults to false.

    \sa pause(), resume()
*/
bool QQuickFrameAnimation::isPaused() const
{
    Q_D(const QQuickFrameAnimation);
    return d->paused;
}

void QQuickFrameAnimation::setPaused(bool paused)
{
    Q_D(QQuickFrameAnimation);
    if (d->paused != paused) {
        d->paused = paused;
        Q_EMIT pausedChanged();
        d->updateState();
    }
}

/*!
    \qmlproperty int QtQuick::FrameAnimation::currentFrame
    \readonly

    This property holds the number of frame updates since the start.
    When the frame animation is restarted, currentFrame starts from \c 0.

    The following example shows how to react on frame updates.

    \code
    FrameAnimation {
        running: true
        onTriggered: {
            // Run code on every frame update.
        }
    }
    \endcode

    This property can also be used for rendering only every nth frame. Consider an
    advanced usage where the UI contains two heavy elements and to reach smooth 60fps
    overall frame rate, you decide to render these heavy elements at 30fps, first one
    on every even frames and second one on every odd frames:

    \code
    FrameAnimation {
        running: true
        onTriggered: {
            if (currentFrame % 2 == 0)
                updateUIElement1();
            else
                updateUIElement2();
       }
    }
    \endcode

    By default, \c frame is 0.
*/
int QQuickFrameAnimation::currentFrame() const
{
    Q_D(const QQuickFrameAnimation);
    return d->currentFrame;
}

/*!
    \qmlproperty qreal QtQuick::FrameAnimation::frameTime
    \readonly

    This property holds the time (in seconds) since the previous frame update.

    The following example shows how to use frameTime to animate item with
    varying speed, adjusting to screen refresh rates and possible fps drops.

    \code
    Rectangle {
        id: rect
        property real speed: 90
        width: 100
        height: 100
        color: "red"
        anchors.centerIn: parent
    }

    FrameAnimation {
        id: frameAnimation
        running: true
        onTriggered: {
            // Rotate the item speed-degrees / second.
            rect.rotation += rect.speed * frameTime
        }
    }
    \endcode

    By default, \c frameTime is 0.
*/
qreal QQuickFrameAnimation::frameTime() const
{
    Q_D(const QQuickFrameAnimation);
    return d->frameTime;
}

/*!
    \qmlproperty qreal QtQuick::FrameAnimation::smoothFrameTime
    \readonly

    This property holds the smoothed time (in seconds) since the previous frame update.

    The following example shows how to use smoothFrameTime to show average fps.

    \code
    Text {
        text: "fps: " + frameAnimation.fps.toFixed(0)
    }

    FrameAnimation {
        id: frameAnimation
        property real fps: smoothFrameTime > 0 ? (1.0 / smoothFrameTime) : 0
        running: true
    }
    \endcode

    By default, \c smoothFrameTime is 0.
*/
qreal QQuickFrameAnimation::smoothFrameTime() const
{
    Q_D(const QQuickFrameAnimation);
    return d->smoothFrameTime;
}

/*!
    \qmlproperty qreal QtQuick::FrameAnimation::elapsedTime
    \readonly

    This property holds the time (in seconds) since the previous start.

    By default, \c elapsedTime is 0.
*/
qreal QQuickFrameAnimation::elapsedTime() const
{
    Q_D(const QQuickFrameAnimation);
    return d->elapsedTime;
}

/*!
    \qmlmethod QtQuick::FrameAnimation::start()
    \brief Starts the frame animation

    If the frame animation is already running, calling this method has no effect.  The
    \c running property will be true following a call to \c start().
*/
void QQuickFrameAnimation::start()
{
    setRunning(true);
}

/*!
    \qmlmethod QtQuick::FrameAnimation::stop()
    \brief Stops the frame animation

    If the frame animation is not running, calling this method has no effect. Both the \c running and
    \c paused properties will be false following a call to \c stop().
*/
void QQuickFrameAnimation::stop()
{
    setRunning(false);
    setPaused(false);
}

/*!
    \qmlmethod QtQuick::FrameAnimation::restart()
    \brief Restarts the frame animation

    If the FrameAnimation is not running it will be started, otherwise it will be
    stopped, reset to initial state and started. The \c running property
    will be true following a call to \c restart().
*/
void QQuickFrameAnimation::restart()
{
    stop();
    start();
}

/*!
    \qmlmethod QtQuick::FrameAnimation::pause()
    \brief Pauses the frame animation

    If the frame animation is already paused or not \c running, calling this method has no effect.
    The \c paused property will be true following a call to \c pause().
*/
void QQuickFrameAnimation::pause()
{
    setPaused(true);
}

/*!
    \qmlmethod QtQuick::FrameAnimation::resume()
    \brief Resumes a paused frame animation

    If the frame animation is not paused or not \c running, calling this method has no effect.
    The \c paused property will be false following a call to \c resume().
*/
void QQuickFrameAnimation::resume()
{
    setPaused(false);
}

/*!
    \qmlmethod QtQuick::FrameAnimation::reset()
    \brief Resets the frame animation properties

    Calling this method resets the \c frame and \c elapsedTime to their initial
    values (0). This method has no effect on \c running or \c paused properties
    and can be called while they are true or false.

    The difference between calling \c reset() and \c restart() is that \c reset()
    will always initialize the properties while \c restart() initializes them only
    at the next frame update which doesn't happen e.g. if \c restart() is
    immediately followed by \c pause().
*/
void QQuickFrameAnimation::reset()
{
    Q_D(QQuickFrameAnimation);
    setElapsedTime(0);
    setCurrentFrame(0);
    d->prevElapsedTimeNs = 0;
    d->elapsedTimer.start();
}

/*!
    \internal
 */
void QQuickFrameAnimation::classBegin()
{
    Q_D(QQuickFrameAnimation);
    d->componentComplete = false;
}

/*!
    \internal
 */
void QQuickFrameAnimation::componentComplete()
{
    Q_D(QQuickFrameAnimation);
    d->componentComplete = true;
    d->updateState();
}

/*!
    \internal
 */
void QQuickFrameAnimation::setCurrentFrame(int frame)
{
    Q_D(QQuickFrameAnimation);
    if (d->currentFrame != frame) {
        d->currentFrame = frame;
        Q_EMIT currentFrameChanged();
    }
}

/*!
    \internal
 */
void QQuickFrameAnimation::setElapsedTime(qreal elapsedTime)
{
    Q_D(QQuickFrameAnimation);
    if (!qFuzzyCompare(d->elapsedTime, elapsedTime)) {
        d->elapsedTime = elapsedTime;
        Q_EMIT elapsedTimeChanged();
    }
}

QT_END_NAMESPACE

#include "moc_qquickframeanimation_p.cpp"
