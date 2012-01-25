/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

#include "qdeclarativenotifier_p.h"
#include "qdeclarativeproperty_p.h"

QT_BEGIN_NAMESPACE

void QDeclarativeNotifier::emitNotify(QDeclarativeNotifierEndpoint *endpoint)
{
    QDeclarativeNotifierEndpoint **oldDisconnected = endpoint->disconnected;
    endpoint->disconnected = &endpoint;
    endpoint->notifying = 1;

    if (endpoint->next)
        emitNotify(endpoint->next);

    if (endpoint) {

        Q_ASSERT(endpoint->callback);
        
        endpoint->callback(endpoint);

        if (endpoint) 
            endpoint->disconnected = oldDisconnected;
    } 

    if (oldDisconnected) *oldDisconnected = endpoint;
    else if (endpoint) endpoint->notifying = 0;
}

void QDeclarativeNotifierEndpoint::connect(QObject *source, int sourceSignal)
{
    disconnect();

    this->source = source;
    this->sourceSignal = sourceSignal;
    QDeclarativePropertyPrivate::flushSignal(source, sourceSignal);
    QDeclarativeData *ddata = QDeclarativeData::get(source, true);
    ddata->addNotify(sourceSignal, this);
}

void QDeclarativeNotifierEndpoint::copyAndClear(QDeclarativeNotifierEndpoint &other)
{
    if (&other == this)
        return;

    other.disconnect();

    other.callback = callback;

    if (!isConnected())
        return;

    other.notifier = notifier;
    other.sourceSignal = sourceSignal;
    other.disconnected = disconnected;
    other.notifying = notifying;
    if (other.disconnected) *other.disconnected = &other;

    if (next) {
        other.next = next;
        next->prev = &other.next;
    }
    other.prev = prev;
    *other.prev = &other;

    prev = 0;
    next = 0;
    disconnected = 0;
    notifier = 0;
    notifying = 0;
    sourceSignal = -1;
}

QT_END_NAMESPACE

