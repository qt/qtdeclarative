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
#ifndef QQuickPARTICLEGROUP
#define QQuickPARTICLEGROUP

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
#include <private/qquickspriteengine_p.h>
#include "qquickparticlesystem_p.h"
#include "qqmlparserstatus.h"

QT_BEGIN_NAMESPACE

class QQuickParticleGroup : public QQuickStochasticState, public QQmlParserStatus
{
    Q_OBJECT
    //### Would setting limits per group be useful? Or clutter the API?
    //Q_PROPERTY(int maximumAlive READ maximumAlive WRITE setMaximumAlive NOTIFY maximumAliveChanged)

    Q_PROPERTY(QQuickParticleSystem* system READ system WRITE setSystem NOTIFY systemChanged)

    //Intercept children requests and assign to the group & system
    Q_PROPERTY(QQmlListProperty<QObject> particleChildren READ particleChildren DESIGNABLE false)//### Hidden property for in-state system definitions - ought not to be used in actual "Sprite" states
    Q_CLASSINFO("DefaultProperty", "particleChildren")
    QML_NAMED_ELEMENT(ParticleGroup)
    Q_INTERFACES(QQmlParserStatus)

public:
    explicit QQuickParticleGroup(QObject* parent = 0);

    QQmlListProperty<QObject> particleChildren();

    int maximumAlive() const
    {
        return m_maximumAlive;
    }

    QQuickParticleSystem* system() const
    {
        return m_system;
    }

public Q_SLOTS:

    void setMaximumAlive(int arg)
    {
        if (m_maximumAlive != arg) {
            m_maximumAlive = arg;
            Q_EMIT maximumAliveChanged(arg);
        }
    }

    void setSystem(QQuickParticleSystem* arg);

    void delayRedirect(QObject* obj);

Q_SIGNALS:

    void maximumAliveChanged(int arg);

    void systemChanged(QQuickParticleSystem* arg);

protected:
    void componentComplete() override;
    void classBegin() override {}

private:

    void performDelayedRedirects();

    int m_maximumAlive;
    QQuickParticleSystem* m_system;
    QList<QObject*> m_delayedRedirects;
};

QT_END_NAMESPACE

#endif
