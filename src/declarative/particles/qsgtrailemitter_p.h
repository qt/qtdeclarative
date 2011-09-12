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

#ifndef FOLLOWEMITTER_H
#define FOLLOWEMITTER_H
#include "qsgparticleemitter_p.h"
#include "qsgparticleaffector_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)


class QSGTrailEmitter : public QSGParticleEmitter
{
    Q_OBJECT
    Q_PROPERTY(QString follow READ follow WRITE setFollow NOTIFY followChanged)
    Q_PROPERTY(int emitRatePerParticle READ particlesPerParticlePerSecond WRITE setParticlesPerParticlePerSecond NOTIFY particlesPerParticlePerSecondChanged)

    //TODO: Document that TrailEmitter's box is where it follows. It emits in a rect centered on the followed particle
    //TODO: A set of properties that can involve the particle size of the followed
    Q_PROPERTY(QSGParticleExtruder* emitShape READ emissonShape WRITE setEmissionShape NOTIFY emissionShapeChanged)
    Q_PROPERTY(qreal emitHeight READ emitterYVariation WRITE setEmitterYVariation NOTIFY emitterYVariationChanged)
    Q_PROPERTY(qreal emitWidth READ emitterXVariation WRITE setEmitterXVariation NOTIFY emitterXVariationChanged)

public:
    explicit QSGTrailEmitter(QSGItem *parent = 0);
    virtual void emitWindow(int timeStamp);
    virtual void reset();

    int particlesPerParticlePerSecond() const
    {
        return m_particlesPerParticlePerSecond;
    }

    qreal emitterXVariation() const
    {
        return m_emitterXVariation;
    }

    qreal emitterYVariation() const
    {
        return m_emitterYVariation;
    }

    QString follow() const
    {
        return m_follow;
    }

    QSGParticleExtruder* emissonShape() const
    {
        return m_emissionExtruder;
    }

signals:
    void emitFollowParticle(QDeclarativeV8Handle group, QDeclarativeV8Handle followed);

    void particlesPerParticlePerSecondChanged(int arg);

    void emitterXVariationChanged(qreal arg);

    void emitterYVariationChanged(qreal arg);

    void followChanged(QString arg);

    void emissionShapeChanged(QSGParticleExtruder* arg);

public slots:

    void setParticlesPerParticlePerSecond(int arg)
    {
        if (m_particlesPerParticlePerSecond != arg) {
            m_particlesPerParticlePerSecond = arg;
            emit particlesPerParticlePerSecondChanged(arg);
        }
    }
    void setEmitterXVariation(qreal arg)
    {
        if (m_emitterXVariation != arg) {
            m_emitterXVariation = arg;
            emit emitterXVariationChanged(arg);
        }
    }

    void setEmitterYVariation(qreal arg)
    {
        if (m_emitterYVariation != arg) {
            m_emitterYVariation = arg;
            emit emitterYVariationChanged(arg);
        }
    }

    void setFollow(QString arg)
    {
        if (m_follow != arg) {
            m_follow = arg;
            emit followChanged(arg);
        }
    }

    void setEmissionShape(QSGParticleExtruder* arg)
    {
        if (m_emissionExtruder != arg) {
            m_emissionExtruder = arg;
            emit emissionShapeChanged(arg);
        }
    }

private slots:
    void recalcParticlesPerSecond();

private:
    QSet<QSGParticleData*> m_pending;
    QVector<qreal> m_lastEmission;
    int m_particlesPerParticlePerSecond;
    qreal m_lastTimeStamp;
    qreal m_emitterXVariation;
    qreal m_emitterYVariation;
    QString m_follow;
    int m_followCount;
    QSGParticleExtruder* m_emissionExtruder;
    QSGParticleExtruder* m_defaultEmissionExtruder;
    bool isEmitFollowConnected();
};

QT_END_NAMESPACE
QT_END_HEADER
#endif // FOLLOWEMITTER_H
