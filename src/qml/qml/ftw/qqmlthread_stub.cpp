// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlthread_p.h"

#include <private/qfieldlist_p.h>
#include <private/qtqmlglobal_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qthread.h>

#if QT_CONFIG(qml_type_loader_thread)
#error "The QQmlThread stub is for the case of !qml_type_loader_thread"
#endif

QT_BEGIN_NAMESPACE

class QQmlThreadPrivate : public QObject
{
public:
    using MessageList = QFieldList<QQmlThread::Message, &QQmlThread::Message::next>;

    QQmlThreadPrivate(QQmlThread *q) : q(q) { setObjectName(QStringLiteral("QQmlThread")); }

    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::User) {
            m_processing = true;

            while (!m_messages.isEmpty()) {
                QQmlThread::Message *message = m_messages.takeFirst();
                message->call(q);
                delete message;
            }

            m_processing = false;
        }
        return QObject::event(e);
    }

    MessageList m_messages;
    QQmlThread *q = nullptr;

    bool m_processing = false; // Set when processing messages
};

QQmlThread::QQmlThread() : d(new QQmlThreadPrivate(this)) {}
QQmlThread::~QQmlThread() { delete d; }

void QQmlThread::startup() {}
void QQmlThread::shutdown() {}

void QQmlThread::lock() {}
void QQmlThread::unlock() {}
void QQmlThread::wakeOne() {}
void QQmlThread::wait() {}

QThread *QQmlThread::thread() const
{
    return d->thread();
}

QObject *QQmlThread::threadObject() const
{
    return d;
}

bool QQmlThread::isThisThread() const
{
    return d->thread()->isCurrentThread();
}

bool QQmlThread::isParentThread() const
{
    return d->thread()->isCurrentThread();
}

void QQmlThread::internalCallMethodInThread(Message *message)
{
    internalCallMethodInMain(message);
}

void QQmlThread::internalCallMethodInMain(Message *message)
{
    message->call(this);
    delete message;
}

void QQmlThread::internalPostMethodToThread(Message *message)
{
    internalPostMethodToMain(message);
}

void QQmlThread::internalPostMethodToMain(Message *message)
{
    const bool wasEmpty = d->m_messages.isEmpty();
    d->m_messages.append(message);
    if (wasEmpty && !d->m_processing)
        QCoreApplication::postEvent(d, new QEvent(QEvent::User));
}

void QQmlThread::waitForNextMessage() {}

void QQmlThread::discardMessages()
{
    while (!d->m_messages.isEmpty())
        delete d->m_messages.takeFirst();
}

QT_END_NAMESPACE
