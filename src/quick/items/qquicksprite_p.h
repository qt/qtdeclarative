/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKSPRITE_P_H
#define QQUICKSPRITE_P_H

#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <QDeclarativeListProperty>
#include "qquickspriteengine_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQuickSprite : public QQuickStochasticState
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    //If frame height or width is not specified, it is assumed to be a single long row of square frames.
    //Otherwise, it can be multiple contiguous rows, when one row runs out the next will be used.
    Q_PROPERTY(int frameHeight READ frameHeight WRITE setFrameHeight NOTIFY frameHeightChanged)
    Q_PROPERTY(int frameWidth READ frameWidth WRITE setFrameWidth NOTIFY frameWidthChanged)

public:
    explicit QQuickSprite(QObject *parent = 0);

    QUrl source() const
    {
        return m_source;
    }

    int frameHeight() const
    {
        return m_frameHeight;
    }

    int frameWidth() const
    {
        return m_frameWidth;
    }


signals:

    void sourceChanged(QUrl arg);

    void frameHeightChanged(int arg);

    void frameWidthChanged(int arg);

public slots:

    void setSource(QUrl arg)
    {
        if (m_source != arg) {
            m_source = arg;
            emit sourceChanged(arg);
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


private:
    friend class QSGImageParticle;
    friend class QQuickSpriteEngine;
    friend class QQuickStochasticEngine;
    int m_generatedCount;
    int m_framesPerRow;
    QUrl m_source;
    int m_frameHeight;
    int m_frameWidth;
    int m_rowY;

};

QT_END_NAMESPACE
QT_END_HEADER
#endif // QQUICKSPRITE_P_H
