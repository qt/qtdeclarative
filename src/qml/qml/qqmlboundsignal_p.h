/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQMLBOUNDSIGNAL_P_H
#define QQMLBOUNDSIGNAL_P_H

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

#include "qqmlexpression.h"

#include <QtCore/qmetaobject.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

class Q_QML_EXPORT QQmlAbstractBoundSignal
{
public:
    QQmlAbstractBoundSignal();
    virtual ~QQmlAbstractBoundSignal();

    virtual int index() const = 0;
    virtual QQmlExpression *expression() const = 0;
    virtual QQmlExpression *setExpression(QQmlExpression *) = 0;
    virtual QObject *object() = 0;

    void addToObject();

private:
    friend class QQmlData;
    friend class QQmlPropertyPrivate;
    friend class QQmlEngineDebugService;
    QQmlAbstractBoundSignal **m_prevSignal;
    QQmlAbstractBoundSignal  *m_nextSignal;
};

class QQmlBoundSignalParameters;
class Q_QML_EXPORT QQmlBoundSignal : public QObject,
                                     public QQmlAbstractBoundSignal
{
public:
    QQmlBoundSignal(QObject *scope, const QMetaMethod &signal, QObject *owner);
    virtual ~QQmlBoundSignal();

    int index() const;

    QQmlExpression *expression() const;
    QQmlExpression *setExpression(QQmlExpression *);
    QObject *object() { return m_owner; }

    bool isEvaluating() const { return m_isEvaluating; }

protected:
    virtual int qt_metacall(QMetaObject::Call c, int id, void **a);

private:
    QQmlExpression *m_expression;
    QMetaMethod m_signal;
    bool m_paramsValid : 1;
    bool m_isEvaluating : 1;
    QQmlBoundSignalParameters *m_params;
    QObject *m_owner;
};

QT_END_NAMESPACE

#endif // QQMLBOUNDSIGNAL_P_H
