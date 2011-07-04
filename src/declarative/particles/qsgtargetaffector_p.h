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

#ifndef QSGTARGETAFFECTOR_H
#define QSGTARGETAFFECTOR_H

#include "qsgparticleaffector_p.h"

class QSGTargetAffector : public QSGParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(int targetX READ targetX WRITE setTargetX NOTIFY targetXChanged)
    Q_PROPERTY(int targetY READ targetY WRITE setTargetY NOTIFY targetYChanged)
    Q_PROPERTY(int targetWidth READ targetWidth WRITE setTargetWidth NOTIFY targetWidthChanged)
    Q_PROPERTY(int targetHeight READ targetHeight WRITE setTargetHeight NOTIFY targetHeightChanged)
    Q_PROPERTY(QSGParticleExtruder* targetShape READ targetShape WRITE setTargetShape NOTIFY targetShapeChanged)
    Q_PROPERTY(int targetTime READ targetTime WRITE setTargetTime NOTIFY targetTimeChanged)

public:
    explicit QSGTargetAffector(QSGItem *parent = 0);

    int targetX() const
    {
        return m_targetX;
    }

    int targetY() const
    {
        return m_targetY;
    }

    int targetWidth() const
    {
        return m_targetWidth;
    }

    int targetHeight() const
    {
        return m_targetHeight;
    }

    QSGParticleExtruder* targetShape() const
    {
        return m_targetShape;
    }

    int targetTime() const
    {
        return m_targetTime;
    }

signals:

    void targetXChanged(int arg);

    void targetYChanged(int arg);

    void targetWidthChanged(int arg);

    void targetHeightChanged(int arg);

    void targetShapeChanged(QSGParticleExtruder* arg);

    void targetTimeChanged(int arg);

public slots:
    void setTargetX(int arg)
    {
        if (m_targetX != arg) {
            m_targetX = arg;
            emit targetXChanged(arg);
        }
    }

    void setTargetY(int arg)
    {
        if (m_targetY != arg) {
            m_targetY = arg;
            emit targetYChanged(arg);
        }
    }

    void setTargetWidth(int arg)
    {
        if (m_targetWidth != arg) {
            m_targetWidth = arg;
            emit targetWidthChanged(arg);
        }
    }

    void setTargetHeight(int arg)
    {
        if (m_targetHeight != arg) {
            m_targetHeight = arg;
            emit targetHeightChanged(arg);
        }
    }

    void setTargetShape(QSGParticleExtruder* arg)
    {
        if (m_targetShape != arg) {
            m_targetShape = arg;
            emit targetShapeChanged(arg);
        }
    }

    void setTargetTime(int arg)
    {
        if (m_targetTime != arg) {
            m_targetTime = arg;
            emit targetTimeChanged(arg);
        }
    }

protected:
    virtual void reset(QSGParticleData*);
    virtual bool affectParticle(QSGParticleData *d, qreal dt);
private:
    int m_targetX;
    int m_targetY;
    int m_targetWidth;
    int m_targetHeight;
    QSGParticleExtruder* m_defaultShape;
    QSGParticleExtruder* m_targetShape;
    int m_targetTime;

    QHash<QPair<int, int>, QPointF> m_targets;
};

#endif // QSGTARGETAFFECTOR_H
