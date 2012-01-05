/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CUSTOMAFFECTOR_H
#define CUSTOMAFFECTOR_H

#include <QObject>
#include "qquickparticlesystem_p.h"
#include "qquickparticleextruder_p.h"
#include "qquickparticleaffector_p.h"
#include "qquickdirection_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQuickCustomAffector : public QQuickParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(bool relative READ relative WRITE setRelative NOTIFY relativeChanged)
    Q_PROPERTY(QQuickDirection *position READ position WRITE setPosition NOTIFY positionChanged RESET positionReset)
    Q_PROPERTY(QQuickDirection *speed READ speed WRITE setSpeed NOTIFY speedChanged RESET speedReset)
    Q_PROPERTY(QQuickDirection *acceleration READ acceleration WRITE setAcceleration NOTIFY accelerationChanged RESET accelerationReset)

public:
    explicit QQuickCustomAffector(QQuickItem *parent = 0);
    virtual void affectSystem(qreal dt);

    QQuickDirection * position() const
    {
        return m_position;
    }

    QQuickDirection * speed() const
    {
        return m_speed;
    }

    QQuickDirection * acceleration() const
    {
        return m_acceleration;
    }

    void positionReset()
    {
        m_position = &m_nullVector;
    }

    void speedReset()
    {
        m_speed = &m_nullVector;
    }

    void accelerationReset()
    {
        m_acceleration = &m_nullVector;
    }

    bool relative() const
    {
        return m_relative;
    }


signals:
    void affectParticles(QDeclarativeV8Handle particles, qreal dt);

    void positionChanged(QQuickDirection * arg);

    void speedChanged(QQuickDirection * arg);

    void accelerationChanged(QQuickDirection * arg);

    void relativeChanged(bool arg);

public slots:
    void setPosition(QQuickDirection * arg)
    {
        if (m_position != arg) {
            m_position = arg;
            emit positionChanged(arg);
        }
    }

    void setSpeed(QQuickDirection * arg)
    {
        if (m_speed != arg) {
            m_speed = arg;
            emit speedChanged(arg);
        }
    }

    void setAcceleration(QQuickDirection * arg)
    {
        if (m_acceleration != arg) {
            m_acceleration = arg;
            emit accelerationChanged(arg);
        }
    }

    void setRelative(bool arg)
    {
        if (m_relative != arg) {
            m_relative = arg;
            emit relativeChanged(arg);
        }
    }

protected:
    bool isAffectConnected();
    virtual bool affectParticle(QQuickParticleData *d, qreal dt);
private:
    void affectProperties(const QList<QQuickParticleData*> particles, qreal dt);
    QQuickDirection * m_position;
    QQuickDirection * m_speed;
    QQuickDirection * m_acceleration;

    QQuickDirection m_nullVector;
    bool m_relative;
};

QT_END_NAMESPACE
QT_END_HEADER
#endif // CUSTOMAFFECTOR_H
