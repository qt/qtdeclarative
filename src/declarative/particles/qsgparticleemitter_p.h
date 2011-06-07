/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
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
#include "qsgstochasticdirection_p.h"

#include <QList>
#include <QPair>
#include <QPointF>
QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGParticleEmitter : public QSGItem
{
    Q_OBJECT
    //###currently goes in emitters OR sets system. Pick one?
    Q_PROPERTY(QSGParticleSystem* system READ system WRITE setSystem NOTIFY systemChanged)
    Q_PROPERTY(QString particle READ particle WRITE setParticle NOTIFY particleChanged)
    Q_PROPERTY(QSGParticleExtruder* shape READ extruder WRITE setExtruder NOTIFY extruderChanged)
    Q_PROPERTY(bool emitting READ emitting WRITE setEmitting NOTIFY emittingChanged)

    Q_PROPERTY(qreal particlesPerSecond READ particlesPerSecond WRITE setParticlesPerSecond NOTIFY particlesPerSecondChanged)
    Q_PROPERTY(int particleDuration READ particleDuration WRITE setParticleDuration NOTIFY particleDurationChanged)
    Q_PROPERTY(int particleDurationVariation READ particleDurationVariation WRITE setParticleDurationVariation NOTIFY particleDurationVariationChanged)
    Q_PROPERTY(int maxParticles READ maxParticleCount WRITE setMaxParticleCount NOTIFY maxParticleCountChanged)

    Q_PROPERTY(qreal particleSize READ particleSize WRITE setParticleSize NOTIFY particleSizeChanged)
    Q_PROPERTY(qreal particleEndSize READ particleEndSize WRITE setParticleEndSize NOTIFY particleEndSizeChanged)
    Q_PROPERTY(qreal particleSizeVariation READ particleSizeVariation WRITE setParticleSizeVariation NOTIFY particleSizeVariationChanged)

    Q_PROPERTY(QSGStochasticDirection *speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(QSGStochasticDirection *acceleration READ acceleration WRITE setAcceleration NOTIFY accelerationChanged)
public:
    explicit QSGParticleEmitter(QSGItem *parent = 0);
    virtual ~QSGParticleEmitter();
    virtual void emitWindow(int timeStamp);

    bool emitting() const
    {
        return m_emitting;
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

    QString particle() const
    {
        return m_particle;
    }

    int particleDurationVariation() const
    {
        return m_particleDurationVariation;
    }

    virtual void componentComplete();
signals:
    void particlesPerSecondChanged(qreal);
    void particleDurationChanged(int);
    void emittingChanged(bool);

    void systemChanged(QSGParticleSystem* arg);

    void particleChanged(QString arg);

    void particleDurationVariationChanged(int arg);

    void extruderChanged(QSGParticleExtruder* arg);

    void particleSizeChanged(qreal arg);

    void particleEndSizeChanged(qreal arg);

    void particleSizeVariationChanged(qreal arg);

    void speedChanged(QSGStochasticDirection * arg);

    void accelerationChanged(QSGStochasticDirection * arg);

    void maxParticleCountChanged(int arg);
    void particleCountChanged();

public slots:
    void pulse(qreal seconds);
    void burst(int num);

    void setEmitting(bool arg);

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

       void setParticle(QString arg)
       {
           if (m_particle != arg) {
               m_particle = arg;
               emit particleChanged(arg);
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

       void setSpeed(QSGStochasticDirection * arg)
       {
           if (m_speed != arg) {
               m_speed = arg;
               emit speedChanged(arg);
           }
       }

       void setAcceleration(QSGStochasticDirection * arg)
       {
           if (m_acceleration != arg) {
               m_acceleration = arg;
               emit accelerationChanged(arg);
           }
       }

       void setMaxParticleCount(int arg);

public:
       int particleCount() const;

       virtual void reset(){;}
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

       QSGStochasticDirection * speed() const
       {
           return m_speed;
       }

       QSGStochasticDirection * acceleration() const
       {
           return m_acceleration;
       }

       int maxParticleCount() const
       {
           return m_maxParticleCount;
       }

protected:
       qreal m_particlesPerSecond;
       int m_particleDuration;
       int m_particleDurationVariation;
       bool m_emitting;
       QSGParticleSystem* m_system;
       QString m_particle;
       QSGParticleExtruder* m_extruder;
       QSGParticleExtruder* m_defaultExtruder;
       QSGParticleExtruder* effectiveExtruder();
       QSGStochasticDirection * m_speed;
       QSGStochasticDirection * m_acceleration;
       qreal m_particleSize;
       qreal m_particleEndSize;
       qreal m_particleSizeVariation;

       int m_burstLeft;//TODO: Rename to pulse
       QList<QPair<int, QPointF > > m_burstQueue;
       int m_maxParticleCount;
private:
       QSGStochasticDirection m_nullVector;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // PARTICLEEMITTER_H
