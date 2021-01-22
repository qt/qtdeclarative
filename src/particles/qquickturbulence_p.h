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

#ifndef TURBULENCEAFFECTOR_H
#define TURBULENCEAFFECTOR_H

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
#include <QQmlListProperty>

QT_BEGIN_NAMESPACE

class QQuickParticlePainter;

class QQuickTurbulenceAffector : public QQuickParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(qreal strength READ strength WRITE setStrength NOTIFY strengthChanged)
    Q_PROPERTY(QUrl noiseSource READ noiseSource WRITE setNoiseSource NOTIFY noiseSourceChanged)
    QML_NAMED_ELEMENT(Turbulence)

public:
    explicit QQuickTurbulenceAffector(QQuickItem *parent = 0);
    ~QQuickTurbulenceAffector();
    void affectSystem(qreal dt) override;

    qreal strength() const
    {
        return m_strength;
    }

    QUrl noiseSource() const
    {
        return m_noiseSource;
    }
Q_SIGNALS:

    void strengthChanged(qreal arg);

    void noiseSourceChanged(QUrl arg);

public Q_SLOTS:

    void setStrength(qreal arg)
    {
        if (m_strength != arg) {
            m_strength = arg;
            Q_EMIT strengthChanged(arg);
        }
    }

    void setNoiseSource(QUrl arg)
    {
        if (m_noiseSource != arg) {
            m_noiseSource = arg;
            Q_EMIT noiseSourceChanged(arg);
            initializeGrid();
        }
    }

protected:
    void geometryChanged(const QRectF &newGeometry,
                         const QRectF &oldGeometry) override;
private:
    void ensureInit();
    void mapUpdate();
    void initializeGrid();
    qreal boundsRespectingField(int x, int y);
    qreal m_strength;
    qreal m_lastT;
    int m_gridSize;
    qreal** m_field;
    QPointF** m_vectorField;
    bool m_inited;
    QUrl m_noiseSource;
};

QT_END_NAMESPACE
#endif // TURBULENCEAFFECTOR_H
