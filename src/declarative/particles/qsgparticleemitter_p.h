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

#ifndef PARTICLEEMITTER_H
#define PARTICLEEMITTER_H

#include <QSGItem>
#include <QDebug>
#include "qsgparticlesystem_p.h"
#include "qsgparticleextruder_p.h"
#include "qsgdirection_p.h"

#include <QList>
#include <QPair>
#include <QPointF>
QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGParticleEmitter : public QSGItem
{
    Q_OBJECT
    Q_PROPERTY(QSGParticleSystem* system READ system WRITE setSystem NOTIFY systemChanged)
    Q_PROPERTY(QString group READ group WRITE setGroup NOTIFY groupChanged)
    Q_PROPERTY(QSGParticleExtruder* shape READ extruder WRITE setExtruder NOTIFY extruderChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int startTime READ startTime WRITE setStartTime NOTIFY startTimeChanged)

    Q_PROPERTY(qreal emitRate READ particlesPerSecond WRITE setParticlesPerSecond NOTIFY particlesPerSecondChanged)
    Q_PROPERTY(int lifeSpan READ particleDuration WRITE setParticleDuration NOTIFY particleDurationChanged)
    Q_PROPERTY(int lifeSpanVariation READ particleDurationVariation WRITE setParticleDurationVariation NOTIFY particleDurationVariationChanged)
    Q_PROPERTY(int maximumEmitted READ maxParticleCount WRITE setMaxParticleCount NOTIFY maximumEmittedChanged)

    Q_PROPERTY(qreal size READ particleSize WRITE setParticleSize NOTIFY particleSizeChanged)
    Q_PROPERTY(qreal endSize READ particleEndSize WRITE setParticleEndSize NOTIFY particleEndSizeChanged)
    Q_PROPERTY(qreal sizeVariation READ particleSizeVariation WRITE setParticleSizeVariation NOTIFY particleSizeVariationChanged)

    Q_PROPERTY(QSGDirection *speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(QSGDirection *acceleration READ acceleration WRITE setAcceleration NOTIFY accelerationChanged)
    Q_PROPERTY(qreal speedFromMovement READ speedFromMovement WRITE setSpeedFromMovement NOTIFY speedFromMovementChanged)

    Q_ENUMS(Lifetime)
public:
    explicit QSGParticleEmitter(QSGItem *parent = 0);
    virtual ~QSGParticleEmitter();
    virtual void emitWindow(int timeStamp);

    enum Lifetime {
        InfiniteLife = QSGParticleSystem::maxLife
    };

    bool enabled() const
    {
        return m_enabled;
    }

    qreal particlesPerSecond() const
    {
        return m_particlesPerSecond;
    }

    int particleDuration() const
    {
        return m_particleDuration;
    }

    QSGParticleSystem* system() const
    {
        return m_system;
    }

    QString group() const
    {
        return m_group;
    }

    int particleDurationVariation() const
    {
        return m_particleDurationVariation;
    }

    qreal speedFromMovement() const { return m_speed_from_movement; }
    void setSpeedFromMovement(qreal s);
    virtual void componentComplete();
signals:
    void emitParticles(QDeclarativeV8Handle particles);
    void particlesPerSecondChanged(qreal);
    void particleDurationChanged(int);
    void enabledChanged(bool);

    void systemChanged(QSGParticleSystem* arg);

    void groupChanged(QString arg);

    void particleDurationVariationChanged(int arg);

    void extruderChanged(QSGParticleExtruder* arg);

    void particleSizeChanged(qreal arg);

    void particleEndSizeChanged(qreal arg);

    void particleSizeVariationChanged(qreal arg);

    void speedChanged(QSGDirection * arg);

    void accelerationChanged(QSGDirection * arg);

    void maximumEmittedChanged(int arg);
    void particleCountChanged();

    void speedFromMovementChanged();

    void startTimeChanged(int arg);

public slots:
    void pulse(int milliseconds);
    void burst(int num);
    void burst(int num, qreal x, qreal y);

    void setEnabled(bool arg);

    void setParticlesPerSecond(qreal arg)
    {
        if (m_particlesPerSecond != arg) {
            m_particlesPerSecond = arg;
            emit particlesPerSecondChanged(arg);
        }
    }

    void setParticleDuration(int arg)
    {
        if (m_particleDuration != arg) {
            m_particleDuration = arg;
            emit particleDurationChanged(arg);
        }
    }

    void setSystem(QSGParticleSystem* arg)
    {
        if (m_system != arg) {
            m_system = arg;
            m_system->registerParticleEmitter(this);
            emit systemChanged(arg);
        }
    }

    void setGroup(QString arg)
    {
        if (m_group != arg) {
            m_group = arg;
            emit groupChanged(arg);
        }
    }

    void setParticleDurationVariation(int arg)
    {
        if (m_particleDurationVariation != arg) {
            m_particleDurationVariation = arg;
            emit particleDurationVariationChanged(arg);
        }
    }
    void setExtruder(QSGParticleExtruder* arg)
    {
        if (m_extruder != arg) {
            m_extruder = arg;
            emit extruderChanged(arg);
        }
    }

    void setParticleSize(qreal arg)
    {
        if (m_particleSize != arg) {
            m_particleSize = arg;
            emit particleSizeChanged(arg);
        }
    }

    void setParticleEndSize(qreal arg)
    {
        if (m_particleEndSize != arg) {
            m_particleEndSize = arg;
            emit particleEndSizeChanged(arg);
        }
    }

    void setParticleSizeVariation(qreal arg)
    {
        if (m_particleSizeVariation != arg) {
            m_particleSizeVariation = arg;
            emit particleSizeVariationChanged(arg);
        }
    }

    void setSpeed(QSGDirection * arg)
    {
        if (m_speed != arg) {
            m_speed = arg;
            emit speedChanged(arg);
        }
    }

    void setAcceleration(QSGDirection * arg)
    {
        if (m_acceleration != arg) {
            m_acceleration = arg;
            emit accelerationChanged(arg);
        }
    }

    void setMaxParticleCount(int arg);

    void setStartTime(int arg)
    {
        if (m_startTime != arg) {
            m_startTime = arg;
            emit startTimeChanged(arg);
        }
    }

       virtual void reset();
public:
       int particleCount() const;

       QSGParticleExtruder* extruder() const
       {
           return m_extruder;
       }

       qreal particleSize() const
       {
           return m_particleSize;
       }

       qreal particleEndSize() const
       {
           return m_particleEndSize;
       }

       qreal particleSizeVariation() const
       {
           return m_particleSizeVariation;
       }

       QSGDirection * speed() const
       {
           return m_speed;
       }

       QSGDirection * acceleration() const
       {
           return m_acceleration;
       }

       int maxParticleCount() const
       {
           return m_maxParticleCount;
       }

       int startTime() const
       {
           return m_startTime;
       }

protected:
       qreal m_particlesPerSecond;
       int m_particleDuration;
       int m_particleDurationVariation;
       bool m_enabled;
       QSGParticleSystem* m_system;
       QString m_group;
       QSGParticleExtruder* m_extruder;
       QSGParticleExtruder* m_defaultExtruder;
       QSGParticleExtruder* effectiveExtruder();
       QSGDirection * m_speed;
       QSGDirection * m_acceleration;
       qreal m_particleSize;
       qreal m_particleEndSize;
       qreal m_particleSizeVariation;

       qreal m_speedFromMovement;
       int m_startTime;
       bool m_overwrite;

       int m_pulseLeft;
       QList<QPair<int, QPointF > > m_burstQueue;
       int m_maxParticleCount;

       //Used in default implementation, but might be useful
       qreal m_speed_from_movement;

       int m_emitCap;
       bool m_reset_last;
       qreal m_last_timestamp;
       qreal m_last_emission;

       QPointF m_last_emitter;
       QPointF m_last_last_emitter;
       QPointF m_last_last_last_emitter;

       bool isEmitConnected();
private:
       QSGDirection m_nullVector;

};

QT_END_NAMESPACE

QT_END_HEADER

#endif // PARTICLEEMITTER_H
