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
{
}

QQuickAnimatorController::~QQuickAnimatorController()
{
    qDeleteAll(activeRootAnimations);
}

void QQuickAnimatorController::itemDestroyed(QObject *o)
{
    deletedSinceLastFrame << (QQuickItem *) o;
}

void QQuickAnimatorController::advance()
{
    bool running = false;
    for (QSet<QAbstractAnimationJob *>::const_iterator it = activeRootAnimations.constBegin();
         !running && it != activeRootAnimations.constEnd(); ++it) {
        if ((*it)->isRunning())
            running = true;
    }

    // It was tempting to only run over the active animations, but we need to push
    // the values for the transforms that finished in the last frame and those will
    // have been removed already...
    for (QHash<QQuickItem *, QQuickTransformAnimatorJob::Helper *>::const_iterator it = transforms.constBegin();
         it != transforms.constEnd(); ++it) {
        (*it)->apply();
    }

    if (running)
        window->update();
}

static void qquick_initialize_helper(QAbstractAnimationJob *job, QQuickAnimatorController *c)
{
    if (job->isRenderThreadJob()) {
        QQuickAnimatorJob *j = static_cast<QQuickAnimatorJob *>(job);
        if (!j->target())
            return;
        else if (c->deletedSinceLastFrame.contains(j->target()))
            j->targetWasDeleted();
        else
            j->initialize(c);
    } else if (job->isGroup()) {
        QAnimationGroupJob *g = static_cast<QAnimationGroupJob *>(job);
        for (QAbstractAnimationJob *a = g->firstChild(); a; a = a->nextSibling())
            qquick_initialize_helper(a, c);
    }
}

void QQuickAnimatorController::beforeNodeSync()
{
    // Force a render pass if we are adding new animations
    // so that advance will be called..
    if (starting.size())
        window->update();

    for (int i=0; i<starting.size(); ++i) {
        QAbstractAnimationJob *job = starting.at(i);
        job->addAnimationChangeListener(this, QAbstractAnimationJob::StateChange);
        qquick_initialize_helper(job, this);
        job->start();
    }
    starting.clear();

    for (QSet<QQuickAnimatorJob *>::const_iterator it = activeLeafAnimations.constBegin();
         it != activeLeafAnimations.constEnd(); ++it) {
        QQuickAnimatorJob *job = *it;
        if (!job->target())
            continue;
        else if (deletedSinceLastFrame.contains(job->target()))
            job->targetWasDeleted();
        else if (job->isTransform()) {
            QQuickTransformAnimatorJob *xform = static_cast<QQuickTransformAnimatorJob *>(*it);
            xform->transformHelper()->sync();
        }
    }
    deletedSinceLastFrame.clear();
}

void QQuickAnimatorController::afterNodeSync()
{
    for (QSet<QQuickAnimatorJob *>::const_iterator it = activeLeafAnimations.constBegin();
         it != activeLeafAnimations.constEnd(); ++it) {
        QQuickAnimatorJob *job = *it;
        if (job->isUniform() && job->target()) {
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
