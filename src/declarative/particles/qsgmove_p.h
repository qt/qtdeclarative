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

#ifndef MOVEAFFECTOR_H
#define MOVEAFFECTOR_H
#include "qsgparticleaffector_p.h"
#include "qsgdirection_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)


class QSGMoveAffector : public QSGParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(bool relative READ relative WRITE setRelative NOTIFY relativeChanged)
    Q_PROPERTY(QSGDirection *position READ position WRITE setPosition NOTIFY positionChanged RESET positionReset)
    Q_PROPERTY(QSGDirection *speed READ speed WRITE setSpeed NOTIFY speedChanged RESET speedReset)
    Q_PROPERTY(QSGDirection *acceleration READ acceleration WRITE setAcceleration NOTIFY accelerationChanged RESET accelerationReset)

public:
    explicit QSGMoveAffector(QQuickItem *parent = 0);
    QSGDirection * position() const
    {
        return m_position;
    }

    QSGDirection * speed() const
    {
        return m_speed;
    }

    QSGDirection * acceleration() const
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

protected:
    virtual bool affectParticle(QSGParticleData *d, qreal dt);
signals:
    void positionChanged(QSGDirection * arg);

    void speedChanged(QSGDirection * arg);

    void accelerationChanged(QSGDirection * arg);

    void relativeChanged(bool arg);

public slots:
    void setPosition(QSGDirection * arg)
    {
        if (m_position != arg) {
            m_position = arg;
            emit positionChanged(arg);
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

    void setRelative(bool arg)
    {
        if (m_relative != arg) {
            m_relative = arg;
            emit relativeChanged(arg);
        }
    }

private slots:
private:
    QSGDirection * m_position;
    QSGDirection * m_speed;
    QSGDirection * m_acceleration;

    QSGDirection m_nullVector;
    bool m_relative;
};

QT_END_NAMESPACE
QT_END_HEADER
#endif // MOVEAFFECTOR_H
