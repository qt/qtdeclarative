/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef GRAVITYAFFECTOR_H
#define GRAVITYAFFECTOR_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//
#include "qquickparticleaffector_p.h"
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickGravityAffector : public QQuickParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(qreal magnitude READ magnitude WRITE setMagnitude NOTIFY magnitudeChanged)
    Q_PROPERTY(qreal acceleration READ magnitude WRITE setAcceleration NOTIFY magnitudeChanged)
    Q_PROPERTY(qreal angle READ angle WRITE setAngle NOTIFY angleChanged)
    QML_NAMED_ELEMENT(Gravity)
public:
    explicit QQuickGravityAffector(QQuickItem *parent = 0);
    qreal magnitude() const;
    qreal angle() const;

protected:
    bool affectParticle(QQuickParticleData *d, qreal dt) override;

Q_SIGNALS:
    void magnitudeChanged(qreal arg);
    void angleChanged(qreal arg);

public Q_SLOTS:
    void setMagnitude(qreal arg);
    void setAcceleration(qreal arg);
    void setAngle(qreal arg);

private:
    qreal m_magnitude;
    qreal m_angle;

    bool m_needRecalc;
    qreal m_dx;
    qreal m_dy;
};

QT_END_NAMESPACE
#endif // GRAVITYAFFECTOR_H
