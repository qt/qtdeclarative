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
#ifndef QSGPARTICLEGROUP
#define QSGPARTICLEGROUP
#include <private/qsgspriteengine_p.h>
#include "qsgparticlesystem_p.h"
#include "qdeclarativeparserstatus.h"

QT_BEGIN_NAMESPACE

class QSGParticleGroup : public QSGStochasticState, public QDeclarativeParserStatus
{
    Q_OBJECT
    //### Would setting limits per group be useful? Or clutter the API?
    //Q_PROPERTY(int maximumAlive READ maximumAlive WRITE setMaximumAlive NOTIFY maximumAliveChanged)

    Q_PROPERTY(QSGParticleSystem* system READ system WRITE setSystem NOTIFY systemChanged)

    //Intercept children requests and assign to the group & system
    Q_PROPERTY(QDeclarativeListProperty<QObject> particleChildren READ particleChildren DESIGNABLE false)//### Hidden property for in-state system definitions - ought not to be used in actual "Sprite" states
    Q_CLASSINFO("DefaultProperty", "particleChildren")
    Q_INTERFACES(QDeclarativeParserStatus)

public:
    explicit QSGParticleGroup(QObject* parent = 0);

    QDeclarativeListProperty<QObject> particleChildren();

    int maximumAlive() const
    {
        return m_maximumAlive;
    }

    QSGParticleSystem* system() const
    {
        return m_system;
    }

public slots:

    void setMaximumAlive(int arg)
    {
        if (m_maximumAlive != arg) {
            m_maximumAlive = arg;
            emit maximumAliveChanged(arg);
        }
    }

    void setSystem(QSGParticleSystem* arg);

    void delayRedirect(QObject* obj);

signals:

    void maximumAliveChanged(int arg);

    void systemChanged(QSGParticleSystem* arg);

protected:
    virtual void componentComplete();
    virtual void classBegin(){;}

private:

    void performDelayedRedirects();

    int m_maximumAlive;
    QSGParticleSystem* m_system;
    QList<QObject*> m_delayedRedirects;
};

QT_END_NAMESPACE

#endif
