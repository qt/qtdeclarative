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

#ifndef TURBULENCEAFFECTOR_H
#define TURBULENCEAFFECTOR_H
#include "qquickparticleaffector_p.h"
#include <QDeclarativeListProperty>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQuickParticlePainter;

class QQuickTurbulenceAffector : public QQuickParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(qreal strength READ strength WRITE setStrength NOTIFY strengthChanged)
    Q_PROPERTY(QUrl noiseSource READ noiseSource WRITE setNoiseSource NOTIFY noiseSourceChanged)
    public:
    explicit QQuickTurbulenceAffector(QQuickItem *parent = 0);
    ~QQuickTurbulenceAffector();
    virtual void affectSystem(qreal dt);

    qreal strength() const
    {
        return m_strength;
    }

    QUrl noiseSource() const
    {
        return m_noiseSource;
    }
signals:

    void strengthChanged(qreal arg);

    void noiseSourceChanged(QUrl arg);

public slots:

    void setStrength(qreal arg)
    {
        if (m_strength != arg) {
            m_strength = arg;
            emit strengthChanged(arg);
        }
    }

    void setNoiseSource(QUrl arg)
    {
        if (m_noiseSource != arg) {
            m_noiseSource = arg;
            emit noiseSourceChanged(arg);
            initializeGrid();
        }
    }

protected:
    virtual void geometryChanged(const QRectF &newGeometry,
                                 const QRectF &oldGeometry);
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
QT_END_HEADER
#endif // TURBULENCEAFFECTOR_H
