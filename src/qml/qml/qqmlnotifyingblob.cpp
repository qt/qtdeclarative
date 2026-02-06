// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlnotifyingblob_p.h"

QT_BEGIN_NAMESPACE

QQmlNotifyingBlob::Callback::~Callback() = default;

void QQmlNotifyingBlob::Callback::ready(QQmlNotifyingBlob *) {}
void QQmlNotifyingBlob::Callback::progress(QQmlNotifyingBlob *, qreal) {}

void QQmlNotifyingBlob::registerCallback(Callback *callback)
{
    assertEngineThread();
    Q_ASSERT(!m_callbacks.contains(callback));
    m_callbacks.append(callback);
}

void QQmlNotifyingBlob::unregisterCallback(Callback *callback)
{
    assertEngineThreadIfRunning();
    Q_ASSERT(m_callbacks.contains(callback));
    m_callbacks.removeOne(callback);
    Q_ASSERT(!m_callbacks.contains(callback));
}

void QQmlNotifyingBlob::completed()
{
    assertEngineThread();
    // Notify callbacks
    while (!m_callbacks.isEmpty()) {
        Callback *callback = m_callbacks.takeFirst();
        callback->ready(this);
    }
}

void QQmlNotifyingBlob::downloadProgressChanged(qreal p)
{
    assertEngineThread();

    for (Callback *callback : std::as_const(m_callbacks))
        callback->progress(this, p);
}

QT_END_NAMESPACE
