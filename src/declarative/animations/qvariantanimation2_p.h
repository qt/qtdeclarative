/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QVARIANTANIMATION2_P_H
#define QVARIANTANIMATION2_P_H

#include <QtCore/qeasingcurve.h>
#include "private/qabstractanimation2_p.h"
#include <QtCore/qvector.h>
#include <QtCore/qvariant.h>
#include <QtCore/qpair.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QVariantAnimation2Private;
class Q_CORE_EXPORT QVariantAnimation2 : public QAbstractAnimation2
{
    Q_OBJECT
public:
    typedef QPair<qreal, qreal> KeyValue;
    typedef QVector<KeyValue> KeyValues;

    QVariantAnimation2(QObject *parent = 0);
    ~QVariantAnimation2();

    qreal keyValueAt(qreal step) const;
    void setKeyValueAt(qreal step, const qreal &value);

    KeyValues keyValues() const;
    void setKeyValues(const KeyValues &values);

    qreal currentValue() const;

    int duration() const;
    void setDuration(int msecs);

    QEasingCurve easingCurve() const;
    void setEasingCurve(const QEasingCurve &easing);

Q_SIGNALS:
    void valueChanged(const QVariant &value);

protected:
    QVariantAnimation2(QVariantAnimation2Private &dd, QObject *parent = 0);
    bool event(QEvent *event);

    void updateCurrentTime(int);
    void updateState(QAbstractAnimation2::State newState, QAbstractAnimation2::State oldState);

    virtual void updateCurrentValue(const qreal &value) = 0;
    virtual qreal interpolated(const qreal &from, const qreal &to, qreal progress) const;

    Q_DISABLE_COPY(QVariantAnimation2)
    Q_DECLARE_PRIVATE(QVariantAnimation2)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif //QVARIANTANIMATION2_P_H
