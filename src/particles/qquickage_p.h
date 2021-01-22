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

#ifndef KILLAFFECTOR_H
#define KILLAFFECTOR_H

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

QT_BEGIN_NAMESPACE

class QQuickAgeAffector : public QQuickParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(int lifeLeft READ lifeLeft WRITE setLifeLeft NOTIFY lifeLeftChanged)
    Q_PROPERTY(bool advancePosition READ advancePosition WRITE setAdvancePosition NOTIFY advancePositionChanged)
    QML_NAMED_ELEMENT(Age)

public:
    explicit QQuickAgeAffector(QQuickItem *parent = 0);

    int lifeLeft() const
    {
        return m_lifeLeft;
    }

    bool advancePosition() const
    {
        return m_advancePosition;
    }

protected:
    bool affectParticle(QQuickParticleData *d, qreal dt) override;

Q_SIGNALS:
    void lifeLeftChanged(int arg);
    void advancePositionChanged(bool arg);

public Q_SLOTS:
    void setLifeLeft(int arg)
    {
        if (m_lifeLeft != arg) {
            m_lifeLeft = arg;
            Q_EMIT lifeLeftChanged(arg);
        }
    }

    void setAdvancePosition(bool arg)
    {
        if (m_advancePosition != arg) {
            m_advancePosition = arg;
            Q_EMIT advancePositionChanged(arg);
        }
    }

private:
    int m_lifeLeft;
    bool m_advancePosition;
};

QT_END_NAMESPACE
#endif // KILLAFFECTOR_H
