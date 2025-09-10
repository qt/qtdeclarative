// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Gunnar Sletta <gunnar@sletta.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickanimatorcontroller_p.h"

#include <private/qquickitem_p.h>
#include <private/qsgrenderloop_p.h>

#include <private/qanimationgroupjob_p.h>

#include <QtGui/qscreen.h>

#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

QQuickAnimatorController::~QQuickAnimatorController()
{
}

QQuickAnimatorController::QQuickAnimatorController(QQuickWindow *window)
    : m_window(window)
{
}

static void qquickanimator_invalidate_jobs(QAbstractAnimationJob *job)
{
    if (job->isRenderThreadJob()) {
        static_cast<QQuickAnimatorJob *>(job)->invalidate();
    } else if (job->isGroup()) {
        for (QAbstractAnimationJob *a : *static_cast<QAnimationGroupJob *>(job)->children())
            qquickanimator_invalidate_jobs(a);
    }
}

void QQuickAnimatorController::windowNodesDestroyed()
{
    for (const QSharedPointer<QAbstractAnimationJob> &toStop : std::as_const(m_rootsPendingStop)) {
        qquickanimator_invalidate_jobs(toStop.data());
        toStop->stop();
    }
    m_rootsPendingStop.clear();

    // Clear animation roots and iterate over a temporary to avoid that job->stop()
    // modifies the m_animationRoots and messes with our iteration
    const auto roots = m_animationRoots;
    m_animationRoots.clear();
    for (const QSharedPointer<QAbstractAnimationJob> &job : roots) {
        qquickanimator_invalidate_jobs(job.data());

        // Stop it and add it to the list of pending start so it might get
        // started later on.
        job->stop();
        m_rootsPendingStart.insert(job);
    }
}

void QQuickAnimatorController::advance()
{
    bool running = false;
    for (const QSharedPointer<QAbstractAnimationJob> &job : std::as_const(m_animationRoots)) {
        if (job->isRunning()) {
            running = true;
            break;
        }
    }

    for (QQuickAnimatorJob *job : std::as_const(m_runningAnimators))
        job->commit();

    if (running)
        m_window->update();
}

static void qquickanimator_sync_before_start(QAbstractAnimationJob *job)
{
    if (job->isRenderThreadJob()) {
        static_cast<QQuickAnimatorJob *>(job)->preSync();
    } else if (job->isGroup()) {
        for (QAbstractAnimationJob *a : *static_cast<QAnimationGroupJob *>(job)->children())
            qquickanimator_sync_before_start(a);
    }
}

void QQuickAnimatorController::beforeNodeSync()
{
    for (const QSharedPointer<QAbstractAnimationJob> &toStop : std::as_const(m_rootsPendingStop)) {
        toStop->stop();
        m_animationRoots.remove(toStop.data());
    }
    m_rootsPendingStop.clear();


    for (QQuickAnimatorJob *job : std::as_const(m_runningAnimators))
        job->preSync();

    // Start pending jobs
    for (const QSharedPointer<QAbstractAnimationJob> &job : std::as_const(m_rootsPendingStart)) {
        Q_ASSERT(!job->isRunning());

        // We want to make sure that presync is called before
        // updateAnimationTime is called the very first time, so before
        // starting a tree of jobs, we go through it and call preSync on all
        // its animators.
        qquickanimator_sync_before_start(job.data());

        // The start the job..
        job->start();
        m_animationRoots.insert(job.data(), job);
    }
    m_rootsPendingStart.clear();

    // Issue an update directly on the window to force another render pass.
    if (m_animationRoots.size())
        m_window->update();
}

void QQuickAnimatorController::afterNodeSync()
{
    for (QQuickAnimatorJob *job : std::as_const(m_runningAnimators))
        job->postSync();
}

void QQuickAnimatorController::animationFinished(QAbstractAnimationJob *job)
{
     m_animationRoots.remove(job);
}

void QQuickAnimatorController::animationStateChanged(QAbstractAnimationJob *job,
                                                     QAbstractAnimationJob::State newState,
                                                     QAbstractAnimationJob::State oldState)
{
    Q_ASSERT(job->isRenderThreadJob());
    QQuickAnimatorJob *animator = static_cast<QQuickAnimatorJob *>(job);
    if (newState == QAbstractAnimationJob::Running) {
        m_runningAnimators.insert(animator);
    } else if (oldState == QAbstractAnimationJob::Running) {
        animator->commit();
        m_runningAnimators.remove(animator);
    }
}


void QQuickAnimatorController::requestSync()
{
    // Force a "sync" pass as the newly started animation needs to sync properties from GUI.
    m_window->maybeUpdate();
}

// All this is being executed on the GUI thread while the animator controller
// is locked.
void QQuickAnimatorController::start_helper(QAbstractAnimationJob *job)
{
    if (job->isRenderThreadJob()) {
        QQuickAnimatorJob *j = static_cast<QQuickAnimatorJob *>(job);
        j->addAnimationChangeListener(this, QAbstractAnimationJob::StateChange);
        j->initialize(this);
    } else if (job->isGroup()) {
        for (QAbstractAnimationJob *a : *static_cast<QAnimationGroupJob *>(job)->children())
            start_helper(a);
    }
}

// Called by the proxy when it is time to kick off an animation job
void QQuickAnimatorController::start(const QSharedPointer<QAbstractAnimationJob> &job)
{
    m_rootsPendingStart.insert(job);
    m_rootsPendingStop.remove(job);
    job->addAnimationChangeListener(this, QAbstractAnimationJob::Completion);
    start_helper(job.data());
    requestSync();
}


// Called by the proxy when it is time to stop an animation job.
void QQuickAnimatorController::cancel(const QSharedPointer<QAbstractAnimationJob> &job)
{
    m_rootsPendingStart.remove(job);
    m_rootsPendingStop.insert(job);
}


QT_END_NAMESPACE

#include "moc_qquickanimatorcontroller_p.cpp"
