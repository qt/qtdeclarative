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

#ifndef QQMLNOTIFIER_P_H
#define QQMLNOTIFIER_P_H

#include "qqmldata_p.h"
#include "qqmlguard_p.h"

QT_BEGIN_NAMESPACE

class QQmlNotifierEndpoint;
class Q_QML_PRIVATE_EXPORT QQmlNotifier
{
public:
    inline QQmlNotifier();
    inline ~QQmlNotifier();
    inline void notify();

private:
    friend class QQmlData;
    friend class QQmlNotifierEndpoint;

    static void emitNotify(QQmlNotifierEndpoint *, void **a);
    QQmlNotifierEndpoint *endpoints;
};

class QQmlEngine;
class QQmlNotifierEndpoint
{
public:
    inline QQmlNotifierEndpoint();
    inline ~QQmlNotifierEndpoint();

    typedef void (*Callback)(QQmlNotifierEndpoint *, void **);
    Callback callback;

    inline bool isConnected();
    inline bool isConnected(QObject *source, int sourceSignal);
    inline bool isConnected(QQmlNotifier *);

    void connect(QObject *source, int sourceSignal, QQmlEngine *engine);
    inline void connect(QQmlNotifier *);
    inline void disconnect();

    inline bool isNotifying() const;
    inline void cancelNotify();

private:
    friend class QQmlData;
    friend class QQmlNotifier;

    union {
        QQmlNotifier *notifier;
        QObject *source;
    };
    unsigned int notifying : 1;
    signed int sourceSignal : 31;
    QQmlNotifierEndpoint **disconnected;
    QQmlNotifierEndpoint  *next;
    QQmlNotifierEndpoint **prev;
};

QQmlNotifier::QQmlNotifier()
: endpoints(0)
{
}

QQmlNotifier::~QQmlNotifier()
{    
    QQmlNotifierEndpoint *endpoint = endpoints;
    while (endpoint) {
        QQmlNotifierEndpoint *n = endpoint;
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

void QQmlNotifier::notify()
{
    void *args[] = { 0 };
    if (endpoints) emitNotify(endpoints, args);
}

QQmlNotifierEndpoint::QQmlNotifierEndpoint()
: callback(0), notifier(0), notifying(0), sourceSignal(-1), disconnected(0), next(0), prev(0)
{
}

QQmlNotifierEndpoint::~QQmlNotifierEndpoint()
{
    disconnect();
}

bool QQmlNotifierEndpoint::isConnected()
{
    return prev != 0;
}

bool QQmlNotifierEndpoint::isConnected(QObject *source, int sourceSignal)
{
    return this->sourceSignal != -1 && this->source == source && this->sourceSignal == sourceSignal;
}

bool QQmlNotifierEndpoint::isConnected(QQmlNotifier *notifier)
{
    return sourceSignal == -1 && this->notifier == notifier;
}

void QQmlNotifierEndpoint::connect(QQmlNotifier *notifier)
{
    disconnect();

    next = notifier->endpoints;
    if (next) { next->prev = &next; }
    notifier->endpoints = this;
    prev = &notifier->endpoints;
    this->notifier = notifier;
}

void QQmlNotifierEndpoint::disconnect()
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
Returns true if a notify is in progress.  This means that the signal or QQmlNotifier
that this endpoing is connected to has been triggered, but this endpoint's callback has not
yet been called.

An in progress notify can be cancelled by calling cancelNotify.
*/
bool QQmlNotifierEndpoint::isNotifying() const
{
    return notifying == 1;
}

/*!
Cancel any notifies that are in progress.
*/
void QQmlNotifierEndpoint::cancelNotify() 
{
    notifying = 0;
    if (disconnected) {
        *disconnected = 0;
        disconnected = 0;
    }
}

QT_END_NAMESPACE

#endif // QQMLNOTIFIER_P_H

