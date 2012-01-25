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

#ifndef QDECLARATIVENOTIFIER_P_H
#define QDECLARATIVENOTIFIER_P_H

#include "qdeclarativedata_p.h"
#include "qdeclarativeguard_p.h"

QT_BEGIN_NAMESPACE

class QDeclarativeNotifierEndpoint;
class Q_DECLARATIVE_EXPORT QDeclarativeNotifier
{
public:
    inline QDeclarativeNotifier();
    inline ~QDeclarativeNotifier();
    inline void notify();

private:
    friend class QDeclarativeData;
    friend class QDeclarativeNotifierEndpoint;

    static void emitNotify(QDeclarativeNotifierEndpoint *);
    QDeclarativeNotifierEndpoint *endpoints;
};

class QDeclarativeNotifierEndpoint
{
public:
    inline QDeclarativeNotifierEndpoint();
    inline ~QDeclarativeNotifierEndpoint();

    typedef void (*Callback)(QDeclarativeNotifierEndpoint *);
    Callback callback;

    inline bool isConnected();
    inline bool isConnected(QObject *source, int sourceSignal);
    inline bool isConnected(QDeclarativeNotifier *);

    void connect(QObject *source, int sourceSignal);
    inline void connect(QDeclarativeNotifier *);
    inline void disconnect();

    inline bool isNotifying() const;
    inline void cancelNotify();

    void copyAndClear(QDeclarativeNotifierEndpoint &other);

private:
    friend class QDeclarativeData;
    friend class QDeclarativeNotifier;

    union {
        QDeclarativeNotifier *notifier;
        QObject *source;
    };
    unsigned int notifying : 1;
    signed int sourceSignal : 31;
    QDeclarativeNotifierEndpoint **disconnected;
    QDeclarativeNotifierEndpoint  *next;
    QDeclarativeNotifierEndpoint **prev;
};

QDeclarativeNotifier::QDeclarativeNotifier()
: endpoints(0)
{
}

QDeclarativeNotifier::~QDeclarativeNotifier()
{    
    QDeclarativeNotifierEndpoint *endpoint = endpoints;
    while (endpoint) {
        QDeclarativeNotifierEndpoint *n = endpoint;
        endpoint = n->next;

        n->next = 0;
        n->prev = 0;
        n->notifier = 0;
        n->sourceSignal = -1;
        if (n->disconnected) *n->disconnected = 0;
        n->disconnected = 0;
    }
    endpoints = 0;
}

void QDeclarativeNotifier::notify()
{
    if (endpoints) emitNotify(endpoints);
}

QDeclarativeNotifierEndpoint::QDeclarativeNotifierEndpoint()
: callback(0), notifier(0), notifying(0), sourceSignal(-1), disconnected(0), next(0), prev(0)
{
}

QDeclarativeNotifierEndpoint::~QDeclarativeNotifierEndpoint()
{
    disconnect();
}

bool QDeclarativeNotifierEndpoint::isConnected()
{
    return prev != 0;
}

bool QDeclarativeNotifierEndpoint::isConnected(QObject *source, int sourceSignal)
{
    return this->sourceSignal != -1 && this->source == source && this->sourceSignal == sourceSignal;
}

bool QDeclarativeNotifierEndpoint::isConnected(QDeclarativeNotifier *notifier)
{
    return sourceSignal == -1 && this->notifier == notifier;
}

void QDeclarativeNotifierEndpoint::connect(QDeclarativeNotifier *notifier)
{
    disconnect();

    next = notifier->endpoints;
    if (next) { next->prev = &next; }
    notifier->endpoints = this;
    prev = &notifier->endpoints;
    this->notifier = notifier;
}

void QDeclarativeNotifierEndpoint::disconnect()
{
    if (next) next->prev = prev;
    if (prev) *prev = next;
    if (disconnected) *disconnected = 0;
    next = 0;
    prev = 0;
    disconnected = 0;
    notifier = 0;
    notifying = 0;
    sourceSignal = -1;
}

/*!
Returns true if a notify is in progress.  This means that the signal or QDeclarativeNotifier
that this endpoing is connected to has been triggered, but this endpoint's callback has not
yet been called.

An in progress notify can be cancelled by calling cancelNotify.
*/
bool QDeclarativeNotifierEndpoint::isNotifying() const
{
    return notifying == 1;
}

/*!
Cancel any notifies that are in progress.
*/
void QDeclarativeNotifierEndpoint::cancelNotify() 
{
    notifying = 0;
    if (disconnected) {
        *disconnected = 0;
        disconnected = 0;
    }
}

QT_END_NAMESPACE

#endif // QDECLARATIVENOTIFIER_P_H

