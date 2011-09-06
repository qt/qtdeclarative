/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
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

#ifndef PARTICLEAFFECTOR_H
#define PARTICLEAFFECTOR_H

#include <QObject>
#include "qsgparticlesystem_p.h"
#include "qsgparticleextruder_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGParticleAffector : public QSGItem
{
    Q_OBJECT
    Q_PROPERTY(QSGParticleSystem* system READ system WRITE setSystem NOTIFY systemChanged)
    Q_PROPERTY(QStringList particles READ particles WRITE setParticles NOTIFY particlesChanged)
    Q_PROPERTY(QStringList whenCollidingWith READ whenCollidingWith WRITE setWhenCollidingWith NOTIFY whenCollidingWithChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool once READ onceOff WRITE setOnceOff NOTIFY onceChanged)
    Q_PROPERTY(QSGParticleExtruder* shape READ shape WRITE setShape NOTIFY shapeChanged)

public:
    explicit QSGParticleAffector(QSGItem *parent = 0);
    virtual void affectSystem(qreal dt);
    virtual void reset(QSGParticleData*);//As some store their own data per particle?
    QSGParticleSystem* system() const
    {
        return m_system;
    }

    QStringList particles() const
    {
        return m_particles;
    }

    bool enabled() const
    {
        return m_enabled;
    }

    bool onceOff() const
    {
        return m_onceOff;
    }

    QSGParticleExtruder* shape() const
    {
        return m_shape;
    }

    QStringList whenCollidingWith() const
    {
        return m_whenCollidingWith;
    }

signals:

    void systemChanged(QSGParticleSystem* arg);

    void particlesChanged(QStringList arg);

    void enabledChanged(bool arg);

    void onceChanged(bool arg);

    void shapeChanged(QSGParticleExtruder* arg);

    void affected(qreal x, qreal y);

    void whenCollidingWithChanged(QStringList arg);

public slots:
void setSystem(QSGParticleSystem* arg)
{
    if (m_system != arg) {
        m_system = arg;
        m_system->registerParticleAffector(this);
        emit systemChanged(arg);
    }
}

void setParticles(QStringList arg)
{
    if (m_particles != arg) {
        m_particles = arg;
        m_updateIntSet = true;
        emit particlesChanged(arg);
    }
}

void setEnabled(bool arg)
{
    if (m_enabled != arg) {
        m_enabled = arg;
        emit enabledChanged(arg);
    }
}

void setOnceOff(bool arg)
{
    if (m_onceOff != arg) {
        m_onceOff = arg;
        m_needsReset = true;
        emit onceChanged(arg);
    }
}

void setShape(QSGParticleExtruder* arg)
{
    if (m_shape != arg) {
        m_shape = arg;
        emit shapeChanged(arg);
    }
}

void setWhenCollidingWith(QStringList arg)
{
    if (m_whenCollidingWith != arg) {
        m_whenCollidingWith = arg;
        emit whenCollidingWithChanged(arg);
    }
}

protected:
    friend class QSGParticleSystem;
    virtual bool affectParticle(QSGParticleData *d, qreal dt);
    bool m_needsReset;//### What is this really saving?
    QSGParticleSystem* m_system;
    QStringList m_particles;
    bool activeGroup(int g) {return m_groups.isEmpty() || m_groups.contains(g);}
    bool m_enabled;
    virtual void componentComplete();
    QPointF m_offset;
    bool isAffectedConnected();
private:
    QSet<int> m_groups;
    QSet<QPair<int, int> > m_onceOffed;
    bool m_updateIntSet;

    bool m_onceOff;

    QSGParticleExtruder* m_shape;

    QStringList m_whenCollidingWith;

    bool isColliding(QSGParticleData* d);
private slots:
    void updateOffsets();
};

QT_END_NAMESPACE
QT_END_HEADER
#endif // PARTICLEAFFECTOR_H
