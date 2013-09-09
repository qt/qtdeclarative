/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef KILLAFFECTOR_H
#define KILLAFFECTOR_H
#include "qquickparticleaffector_p.h"

QT_BEGIN_NAMESPACE

class QQuickAgeAffector : public QQuickParticleAffector
{
    Q_OBJECT
    Q_PROPERTY(int lifeLeft READ lifeLeft WRITE setLifeLeft NOTIFY lifeLeftChanged)
    Q_PROPERTY(bool advancePosition READ advancePosition WRITE setAdvancePosition NOTIFY advancePositionChanged)

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
    virtual bool affectParticle(QQuickParticleData *d, qreal dt);
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
