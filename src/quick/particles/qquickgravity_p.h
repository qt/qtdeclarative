/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef GRAVITYAFFECTOR_H
#define GRAVITYAFFECTOR_H
#include "qquickparticleaffector_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQuickGravityAffector : public QQuickParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(qreal magnitude READ magnitude WRITE setMagnitude NOTIFY magnitudeChanged)
    Q_PROPERTY(qreal acceleration READ magnitude WRITE setAcceleration NOTIFY magnitudeChanged)
    Q_PROPERTY(qreal angle READ angle WRITE setAngle NOTIFY angleChanged)
public:
    explicit QQuickGravityAffector(QQuickItem *parent = 0);
    qreal magnitude() const
    {
        return m_magnitude;
    }

    qreal angle() const
    {
        return m_angle;
    }
protected:
    virtual bool affectParticle(QQuickParticleData *d, qreal dt);
signals:

    void magnitudeChanged(qreal arg);

    void angleChanged(qreal arg);

public slots:
void setAcceleration(qreal arg)
{
    qWarning() << "Gravity::acceleration has been renamed Gravity::magnitude";
    if (m_magnitude != arg) {
        m_magnitude = arg;
        m_needRecalc = true;
        emit magnitudeChanged(arg);
    }
}

void setMagnitude(qreal arg)
{
    if (m_magnitude != arg) {
        m_magnitude = arg;
        m_needRecalc = true;
        emit magnitudeChanged(arg);
    }
}

void setAngle(qreal arg)
{
    if (m_angle != arg) {
        m_angle = arg;
        m_needRecalc = true;
        emit angleChanged(arg);
    }
}

private:
    qreal m_magnitude;
    qreal m_angle;

    bool m_needRecalc;
    qreal m_dx;
    qreal m_dy;
};

QT_END_NAMESPACE
QT_END_HEADER
#endif // GRAVITYAFFECTOR_H
