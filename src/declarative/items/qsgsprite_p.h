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

#ifndef SPRITESTATE_H
#define SPRITESTATE_H

#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <QDeclarativeListProperty>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)


class QSGSprite : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int frames READ frames WRITE setFrames NOTIFY framesChanged)
    //If frame height or width is not specified, it is assumed to be a single long row of frames.
    //Otherwise, it can be multiple contiguous rows, when one row runs out the next will be used.
    Q_PROPERTY(int frameHeight READ frameHeight WRITE setFrameHeight NOTIFY frameHeightChanged)
    Q_PROPERTY(int frameWidth READ frameWidth WRITE setFrameWidth NOTIFY frameWidthChanged)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(int durationVariation READ durationVariance WRITE setDurationVariance NOTIFY durationVarianceChanged)
    Q_PROPERTY(qreal speedModifiesDuration READ speedModifer WRITE setSpeedModifier NOTIFY speedModifierChanged)
    Q_PROPERTY(QVariantMap to READ to WRITE setTo NOTIFY toChanged)

    Q_PROPERTY(QDeclarativeListProperty<QObject> particleChildren READ particleChildren DESIGNABLE false)//### Hidden property for in-state system definitions - ought not to be used in actual "Sprite" states
    Q_CLASSINFO("DefaultProperty", "particleChildren")
public:
    explicit QSGSprite(QObject *parent = 0);

    QDeclarativeListProperty<QObject> particleChildren();

    QUrl source() const
    {
        return m_source;
    }

    int frames() const
    {
        return m_frames;
    }

    int frameHeight() const
    {
        return m_frameHeight;
    }

    int frameWidth() const
    {
        return m_frameWidth;
    }

    int duration() const
    {
        return m_duration;
    }

    QString name() const
    {
        return m_name;
    }

    QVariantMap to() const
    {
        return m_to;
    }

    qreal speedModifer() const
    {
        return m_speedModifier;
    }

    int durationVariance() const
    {
        return m_durationVariance;
    }

    int variedDuration() const;

signals:

    void sourceChanged(QUrl arg);

    void framesChanged(int arg);

    void frameHeightChanged(int arg);

    void frameWidthChanged(int arg);

    void durationChanged(int arg);

    void nameChanged(QString arg);

    void toChanged(QVariantMap arg);

    void speedModifierChanged(qreal arg);

    void durationVarianceChanged(int arg);

    void entered();//### Just playing around - don't expect full state API

public slots:

    void setSource(QUrl arg)
    {
        if (m_source != arg) {
            m_source = arg;
            emit sourceChanged(arg);
        }
    }

    void setFrames(int arg)
    {
        if (m_frames != arg) {
            m_frames = arg;
            emit framesChanged(arg);
        }
    }

    void setFrameHeight(int arg)
    {
        if (m_frameHeight != arg) {
            m_frameHeight = arg;
            emit frameHeightChanged(arg);
        }
    }

    void setFrameWidth(int arg)
    {
        if (m_frameWidth != arg) {
            m_frameWidth = arg;
            emit frameWidthChanged(arg);
        }
    }

    void setDuration(int arg)
    {
        if (m_duration != arg) {
            m_duration = arg;
            emit durationChanged(arg);
        }
    }

    void setName(QString arg)
    {
        if (m_name != arg) {
            m_name = arg;
            emit nameChanged(arg);
        }
    }

    void setTo(QVariantMap arg)
    {
        if (m_to != arg) {
            m_to = arg;
            emit toChanged(arg);
        }
    }

    void setSpeedModifier(qreal arg)
    {
        if (m_speedModifier != arg) {
            m_speedModifier = arg;
            emit speedModifierChanged(arg);
        }
    }

    void setDurationVariance(int arg)
    {
        if (m_durationVariance != arg) {
            m_durationVariance = arg;
            emit durationVarianceChanged(arg);
        }
    }

private:
    friend class QSGImageParticle;
    friend class QSGSpriteEngine;
    int m_generatedCount;
    int m_framesPerRow;
    QUrl m_source;
    int m_frames;
    int m_frameHeight;
    int m_frameWidth;
    int m_duration;
    QString m_name;
    QVariantMap m_to;
    qreal m_speedModifier;
    int m_durationVariance;

};

QT_END_NAMESPACE
QT_END_HEADER
#endif // SPRITESTATE_H
