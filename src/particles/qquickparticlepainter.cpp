// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickparticlepainter_p.h"
#include <QQuickWindow>
#include <QDebug>
QT_BEGIN_NAMESPACE
/*!
    \qmltype ParticlePainter
    \instantiates QQuickParticlePainter
    \inqmlmodule QtQuick.Particles
    \inherits Item
    \brief For specifying how to paint particles.
    \ingroup qtquick-particles

    The default implementation paints nothing. See the subclasses if you want to
    paint something visible.

*/
/*!
    \qmlproperty ParticleSystem QtQuick.Particles::ParticlePainter::system
    This is the system whose particles can be painted by the element.
    If the ParticlePainter is a direct child of a ParticleSystem, it will automatically be associated with it.
*/
/*!
    \qmlproperty list<string> QtQuick.Particles::ParticlePainter::groups
    Which logical particle groups will be painted.

    If empty, it will paint the default particle group ("").
*/
QQuickParticlePainter::QQuickParticlePainter(QQuickItem *parent)
    : QQuickItem(parent)
    , m_system(nullptr)
    , m_count(0)
    , m_pleaseReset(true)
    , m_window(nullptr)
    , m_windowChanged(false)
    , m_groupIdsNeedRecalculation(false)
{
}

void QQuickParticlePainter::itemChange(ItemChange change, const ItemChangeData &data)
{
    if (change == QQuickItem::ItemSceneChange) {
        if (m_window)
            disconnect(m_window, SIGNAL(sceneGraphInvalidated()), this, SLOT(sceneGraphInvalidated()));
        m_window = data.window;
        m_windowChanged = true;
        if (m_window)
            connect(m_window, SIGNAL(sceneGraphInvalidated()), this, SLOT(sceneGraphInvalidated()), Qt::DirectConnection);
    }
    QQuickItem::itemChange(change, data);
}

void QQuickParticlePainter::componentComplete()
{
    if (!m_system && qobject_cast<QQuickParticleSystem*>(parentItem()))
        setSystem(qobject_cast<QQuickParticleSystem*>(parentItem()));
    QQuickItem::componentComplete();
}

void QQuickParticlePainter::recalculateGroupIds() const
{
    if (!m_system) {
        m_groupIds.clear();
        return;
    }

    m_groupIdsNeedRecalculation = false;
    m_groupIds.clear();

    const auto groupList = groups();
    for (const QString &str : groupList) {
        QQuickParticleGroupData::ID groupId = m_system->groupIds.value(str, QQuickParticleGroupData::InvalidID);
        if (groupId == QQuickParticleGroupData::InvalidID) {
            // invalid data, not finished setting up, or whatever. Fallback: do not cache.
            m_groupIdsNeedRecalculation = true;
        } else {
            m_groupIds.append(groupId);
        }
    }
}

void QQuickParticlePainter::setSystem(QQuickParticleSystem *arg)
{
    if (m_system != arg) {
        m_system = arg;
        m_groupIdsNeedRecalculation = true;
        if (m_system){
            m_system->registerParticlePainter(this);
            reset();
        }
        emit systemChanged(arg);
    }
}

void QQuickParticlePainter::setGroups(const QStringList &arg)
{
    if (m_groups != arg) {
        m_groups = arg;
        m_groupIdsNeedRecalculation = true;
        //Note: The system watches this as it has to recalc things when groups change. It will request a reset if necessary
        Q_EMIT groupsChanged(arg);
    }
}

void QQuickParticlePainter::load(QQuickParticleData* d)
{
    initialize(d->groupId, d->index);
    if (m_pleaseReset)
        return;
    m_pendingCommits << qMakePair(d->groupId, d->index);
}

void QQuickParticlePainter::reload(QQuickParticleData* d)
{
    if (m_pleaseReset)
        return;
    m_pendingCommits << qMakePair(d->groupId, d->index);
}

void QQuickParticlePainter::reset()
{
    m_pendingCommits.clear();
    m_pleaseReset = true;
}

void QQuickParticlePainter::setCount(int c)//### TODO: some resizeing so that particles can reallocate on size change instead of recreate
{
    Q_ASSERT(c >= 0); //XXX
    if (c == m_count)
        return;
    m_count = c;
    emit countChanged();
    reset();
}

void QQuickParticlePainter::calcSystemOffset(bool resetPending)
{
    if (!m_system || !parentItem())
        return;
    QPointF lastOffset = m_systemOffset;
    m_systemOffset = -1 * this->mapFromItem(m_system, QPointF(0.0, 0.0));
    if (lastOffset != m_systemOffset && !resetPending){
        //Reload all particles//TODO: Necessary?
        foreach (const QString &g, m_groups){
            int gId = m_system->groupIds[g];
            foreach (QQuickParticleData* d, m_system->groupData[gId]->data)
                reload(d);
        }
    }
}
typedef QPair<int,int> intPair;
void QQuickParticlePainter::performPendingCommits()
{
    calcSystemOffset();
    foreach (intPair p, m_pendingCommits)
        commit(p.first, p.second);
    m_pendingCommits.clear();
}

QT_END_NAMESPACE

#include "moc_qquickparticlepainter_p.cpp"
