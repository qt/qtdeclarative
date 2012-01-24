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

#ifndef SPRITEGOALAFFECTOR_H
#define SPRITEGOALAFFECTOR_H
#include "qquickparticleaffector_p.h"
#include <QtDeclarative/qdeclarativeinfo.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQuickStochasticEngine;

class QQuickSpriteGoalAffector : public QQuickParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(QString goalState READ goalState WRITE setGoalState NOTIFY goalStateChanged)
    Q_PROPERTY(bool jump READ jump WRITE setJump NOTIFY jumpChanged)
    Q_PROPERTY(bool systemStates READ systemStates WRITE setSystemStates NOTIFY systemStatesChanged)
public:
    explicit QQuickSpriteGoalAffector(QQuickItem *parent = 0);

    QString goalState() const
    {
        return m_goalState;
    }

    bool jump() const
    {
        return m_jump;
    }
    bool systemStates() const
    {
        return m_systemStates;
    }

protected:
    virtual bool affectParticle(QQuickParticleData *d, qreal dt);
signals:

    void goalStateChanged(QString arg);

    void jumpChanged(bool arg);

    void systemStatesChanged(bool arg);

public slots:

void setGoalState(QString arg);

void setJump(bool arg)
{
    if (m_jump != arg) {
        m_jump = arg;
        emit jumpChanged(arg);
    }
}

void setSystemStates(bool arg)
{
    if (m_systemStates != arg) {
        //TODO: GroupGoal was added (and this deprecated) Oct 4 - remove it in a few weeks.
        qmlInfo(this) << "systemStates is deprecated and will be removed soon. Use GroupGoal instead.";
        m_systemStates = arg;
        emit systemStatesChanged(arg);
    }
}

private:
    void updateStateIndex(QQuickStochasticEngine* e);
    QString m_goalState;
    int m_goalIdx;
    QQuickStochasticEngine* m_lastEngine;
    bool m_jump;
    bool m_systemStates;

    bool m_notUsingEngine;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // SPRITEGOALAFFECTOR_H
