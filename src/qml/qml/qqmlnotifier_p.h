// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLNOTIFIER_P_H
#define QQMLNOTIFIER_P_H

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

#include <private/qqmlnotifierendpoint_p.h>

QT_BEGIN_NAMESPACE

class QQmlData;

class Q_QML_EXPORT QQmlNotifier
{
public:
    inline QQmlNotifier();
    inline ~QQmlNotifier();
    inline void notify();
    void connect(QQmlNotifierEndpoint *endpoint);

    static void notify(QQmlData *ddata, int notifierIndex);

private:
    friend class QQmlData;
    friend class QQmlNotifierEndpoint;
    friend class QQmlThreadNotifierProxyObject;

    static void emitNotify(QQmlNotifierEndpoint *, void **a);
    QQmlNotifierEndpoint *endpoints = nullptr;
};

QQmlNotifier::QQmlNotifier()
{
}

QQmlNotifier::~QQmlNotifier()
{
    QQmlNotifierEndpoint *endpoint = endpoints;
    while (endpoint) {
        QQmlNotifierEndpoint *n = endpoint;
        endpoint = n->next;
        n->setSender(0x0);
        n->next = nullptr;
        n->prev = nullptr;
        n->sourceSignal = -1;
    }
    endpoints = nullptr;
}

void QQmlNotifier::notify()
{
    void *args[] = { nullptr };
    if (endpoints) emitNotify(endpoints, args);
}

inline void QQmlNotifier::connect(QQmlNotifierEndpoint *endpoint)
{
    endpoint->disconnect();

    endpoint->next = endpoints;
    if (endpoint->next) {
        endpoint->next->prev = &endpoint->next;
    }
    endpoints = endpoint;
    endpoint->prev = &endpoints;
    endpoint->setSender(qintptr(this));
}

QT_END_NAMESPACE

#endif // QQMLNOTIFIER_P_H
