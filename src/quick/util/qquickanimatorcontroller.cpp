/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickanimatorcontroller_p.h"

#include <private/qquickwindow_p.h>
#include <private/qsgrenderloop_p.h>

#include <private/qanimationgroupjob_p.h>

#include <QtGui/qscreen.h>

QT_BEGIN_NAMESPACE

QQuickAnimatorController::QQuickAnimatorController()
    : window(0)
    , driver(0)
{
}

QQuickAnimatorController::~QQuickAnimatorController()
{
    qDeleteAll(activeRootAnimations);
}

void QQuickAnimatorController::advance()
{
    if (driver && driver->isRunning()) {
        // This lock is to prevent conflicts with syncBackCurrentValues
        mutex.lock();
        driver->advance();
        mutex.unlock();
    }

    // The animation system uses a chain of queued connections to
    // start the animation driver and these won't get delievered until,
    // at best, after this frame. We need to track if animations
    // are running here so we can keep on rendering in that case.
    bool running = driver && driver->isRunning();
    for (QSet<QAbstractAnimationJob *>::const_iterator it = activeRootAnimations.constBegin();
         !running && it != activeRootAnimations.constEnd(); ++it) {
        if ((*it)->isRunning())
            running = true;
    }

    for (QSet<QQuickAnimatorJob *>::const_iterator it = activeLeafAnimations.constBegin();
         it != activeLeafAnimations.constEnd(); ++it) {
        if ((*it)->isTransform()) {
            QQuickTransformAnimatorJob *xform = static_cast<QQuickTransformAnimatorJob *>(*it);
            xform->transformHelper()->apply();
        }
    }

    if (running)
        window->update();
}

static void qquick_initialize_helper(QAbstractAnimationJob *job, QQuickAnimatorController *c)
{
    if (job->isRenderThreadJob()) {
        QQuickAnimatorJob *j = static_cast<QQuickAnimatorJob *>(job);
        j->initialize(c);
    } else if (job->isGroup()) {
        QAnimationGroupJob *g = static_cast<QAnimationGroupJob *>(job);
        for (QAbstractAnimationJob *a = g->firstChild(); a; a = a->nextSibling())
            qquick_initialize_helper(a, c);
    }
}

void QQuickAnimatorController::beforeNodeSync()
{
    if (!driver && window->thread() != window->openglContext()->thread()) {
        driver = QQuickWindowPrivate::get(window)->context->createAnimationDriver(this);
        connect(driver, SIGNAL(started()), this, SLOT(animationsStarted()), Qt::DirectConnection);
        connect(driver, SIGNAL(stopped()), this, SLOT(animationsStopped()), Qt::DirectConnection);
        driver->install();

        QUnifiedTimer::instance(true)->setConsistentTiming(QSGRenderLoop::useConsistentTiming());
    }

    // Force a render pass if we are adding new animations
    // so that advance will be called..
    if (starting.size())
        window->update();

    for (int i=0; i<starting.size(); ++i) {
        QAbstractAnimationJob *job = starting.at(i);
        qquick_initialize_helper(job, this);
        job->addAnimationChangeListener(this, QAbstractAnimationJob::StateChange);
        job->start();
    }
    starting.clear();

    for (QSet<QQuickAnimatorJob *>::const_iterator it = activeLeafAnimations.constBegin();
         it != activeLeafAnimations.constEnd(); ++it) {
        if ((*it)->isTransform()) {
            QQuickTransformAnimatorJob *xform = static_cast<QQuickTransformAnimatorJob *>(*it);
            xform->transformHelper()->sync();
        }
    }
}

void QQuickAnimatorController::afterNodeSync()
{
    for (QSet<QQuickAnimatorJob *>::const_iterator it = activeLeafAnimations.constBegin();
         it != activeLeafAnimations.constEnd(); ++it) {
        if ((*it)->isUniform()) {
            QQuickUniformAnimatorJob *job = static_cast<QQuickUniformAnimatorJob *>(*it);
            job->afterNodeSync();
        }
    }
}


void QQuickAnimatorController::startAnimation(QAbstractAnimationJob *job)
{
    mutex.lock();
    starting << job;
    mutex.unlock();
}

void QQuickAnimatorController::animationsStopped()
{
}

void QQuickAnimatorController::animationsStarted()
{
    window->update();
}

void QQuickAnimatorController::animationStateChanged(QAbstractAnimationJob *job,
                                                     QAbstractAnimationJob::State newState,
                                                     QAbstractAnimationJob::State)
{
    if (newState == QAbstractAnimationJob::Running)
        activeRootAnimations << job;
    else
        activeRootAnimations.remove(job);
}

bool QQuickAnimatorController::event(QEvent *e)
{
     if ((int) e->type() == StopAnimation) {
        QAbstractAnimationJob *job = static_cast<Event *>(e)->job;
        mutex.lock();
        starting.removeOne(job);
        mutex.unlock();
        job->stop();
        return true;

    } else if ((uint) e->type() == DeleteAnimation) {
        QAbstractAnimationJob *job = static_cast<Event *>(e)->job;
        mutex.lock();
        starting.removeOne(job);
        mutex.unlock();
        job->stop();
        delete job;
        return true;
    }

    return QObject::event(e);
}

QT_END_NAMESPACE
