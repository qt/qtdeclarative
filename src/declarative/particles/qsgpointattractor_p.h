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

#ifndef ATTRACTORAFFECTOR_H
#define ATTRACTORAFFECTOR_H
#include "qsgparticleaffector_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGAttractorAffector : public QSGParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(qreal strength READ strength WRITE setStrength NOTIFY strengthChanged)
    Q_PROPERTY(qreal pointX READ pointX WRITE setPointX NOTIFY pointXChanged)
    Q_PROPERTY(qreal pointY READ pointY WRITE setPointY NOTIFY pointYChanged)
    Q_PROPERTY(AffectableParameters affectedParameter READ affectedParameter WRITE setAffectedParameter NOTIFY affectedParameterChanged)
    Q_PROPERTY(Proportion proportionalToDistance READ proportionalToDistance WRITE setProportionalToDistance NOTIFY proportionalToDistanceChanged)
    Q_ENUMS(AffectableParameters)
    Q_ENUMS(Proportion)

public:
    enum Proportion{
        Constant,
        Linear,
        Quadratic,
        InverseLinear,
        InverseQuadratic
    };

    enum AffectableParameters {
        Position,
        Velocity,
        Acceleration
    };

    explicit QSGAttractorAffector(QQuickItem *parent = 0);

    qreal strength() const
    {
        return m_strength;
    }

    qreal pointX() const
    {
        return m_x;
    }

    qreal pointY() const
    {
        return m_y;
    }

    AffectableParameters affectedParameter() const
    {
        return m_physics;
    }

    Proportion proportionalToDistance() const
    {
        return m_proportionalToDistance;
    }

signals:

    void strengthChanged(qreal arg);

    void pointXChanged(qreal arg);

    void pointYChanged(qreal arg);

    void affectedParameterChanged(AffectableParameters arg);

    void proportionalToDistanceChanged(Proportion arg);

public slots:
void setStrength(qreal arg)
{
    if (m_strength != arg) {
        m_strength = arg;
        emit strengthChanged(arg);
    }
}

void setPointX(qreal arg)
{
    if (m_x != arg) {
        m_x = arg;
        emit pointXChanged(arg);
    }
}

void setPointY(qreal arg)
{
    if (m_y != arg) {
        m_y = arg;
        emit pointYChanged(arg);
    }
}
void setAffectedParameter(AffectableParameters arg)
{
    if (m_physics != arg) {
        m_physics = arg;
        emit affectedParameterChanged(arg);
    }
}

void setProportionalToDistance(Proportion arg)
{
    if (m_proportionalToDistance != arg) {
        m_proportionalToDistance = arg;
        emit proportionalToDistanceChanged(arg);
    }
}

protected:
    virtual bool affectParticle(QSGParticleData *d, qreal dt);
private:
qreal m_strength;
qreal m_x;
qreal m_y;
AffectableParameters m_physics;
Proportion m_proportionalToDistance;
};

QT_END_NAMESPACE
QT_END_HEADER
#endif // ATTRACTORAFFECTOR_H
