// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickanimationrootitem_p.h"

QT_BEGIN_NAMESPACE

QQuickAnimationRootItem::QQuickAnimationRootItem(QQuickItem *parent) : QQuickItem(parent) { }

bool QQuickAnimationRootItem::paused() const
{
    return m_paused;
}

void QQuickAnimationRootItem::setPaused(bool paused)
{
    if (m_paused == paused)
        return;
    m_paused = paused;
    for (const QPointer<QQuickAbstractAnimation> &anim : std::as_const(m_masterAnimations)) {
        if (anim && anim->isRunning())
            anim->setPaused(paused);
    }
    emit pausedChanged();
}

int QQuickAnimationRootItem::loops() const
{
    return m_loops;
}

void QQuickAnimationRootItem::setLoops(int loops)
{
    if (m_loops == loops)
        return;
    m_loops = loops;
    for (const QPointer<QQuickAbstractAnimation> &anim : std::as_const(m_masterAnimations)) {
        if (!anim)
            continue;
        anim->setLoops(loops);
        if (anim->isRunning())
            anim->restart();
    }
    emit loopsChanged();
}

QVariantMap QQuickAnimationRootItem::markers() const
{
    return m_markers;
}

void QQuickAnimationRootItem::setMarkersData(const QVariantMap &markers)
{
    if (m_markers == markers)
        return;
    m_markers = markers;
    emit markersChanged();
}

qreal QQuickAnimationRootItem::startFrame() const
{
    return m_startFrame;
}

void QQuickAnimationRootItem::setStartFrame(qreal startFrame)
{
    if (m_startFrame == startFrame)
        return;
    m_startFrame = startFrame;
    emit startFrameChanged();
}

qreal QQuickAnimationRootItem::endFrame() const
{
    return m_endFrame;
}

void QQuickAnimationRootItem::setEndFrame(qreal endFrame)
{
    if (m_endFrame == endFrame)
        return;
    m_endFrame = endFrame;
    emit endFrameChanged();
}

qreal QQuickAnimationRootItem::frameRate() const
{
    return m_frameRate;
}

void QQuickAnimationRootItem::setFrameRate(qreal frameRate)
{
    if (m_frameRate == frameRate)
        return;
    m_frameRate = frameRate;
    emit frameRateChanged();
}

qreal QQuickAnimationRootItem::frameCounter() const
{
    return m_frameCounter;
}

void QQuickAnimationRootItem::setFrameCounter(qreal frameCounter)
{
    if (m_frameCounter == frameCounter)
        return;
    m_frameCounter = frameCounter;
    emit frameCounterChanged();
}

void QQuickAnimationRootItem::restart()
{
    for (const QPointer<QQuickAbstractAnimation> &anim : std::as_const(m_masterAnimations)) {
        if (anim)
            anim->restart();
    }
}

void QQuickAnimationRootItem::addMasterAnimation(QQuickAbstractAnimation *anim)
{
    if (!anim)
        return;
    m_masterAnimations.append(anim);
    anim->setLoops(m_loops);
    if (m_paused && anim->isRunning())
        anim->setPaused(true);
}

QT_END_NAMESPACE
