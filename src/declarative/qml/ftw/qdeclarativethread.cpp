/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "qdeclarativethread_p.h"

#include <private/qfieldlist_p.h>

#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

class QDeclarativeThreadPrivate : public QThread
{
public:
    QDeclarativeThreadPrivate(QDeclarativeThread *);
    QDeclarativeThread *q;

    virtual void run();

    inline void lock() { _mutex.lock(); }
    inline void unlock() { _mutex.unlock(); }
    inline void wait() { _wait.wait(&_mutex); }
    inline void wakeOne() { _wait.wakeOne(); }
    inline void wakeAll() { _wait.wakeAll(); }

    quint32 m_threadProcessing:1; // Set when the thread is processing messages
    quint32 m_mainProcessing:1; // Set when the main thread is processing messages
    quint32 m_shutdown:1; // Set by main thread to request a shutdown
    quint32 m_mainThreadWaiting:1; // Set by main thread if it is waiting for the message queue to empty

    typedef QFieldList<QDeclarativeThread::Message, &QDeclarativeThread::Message::next> MessageList;
    MessageList threadList;
    MessageList mainList;

    QDeclarativeThread::Message *mainSync;

    void triggerMainEvent();
    void triggerThreadEvent();

    void mainEvent();
    void threadEvent();

protected:
    virtual bool event(QEvent *); 

private:
    struct MainObject : public QObject { 
        MainObject(QDeclarativeThreadPrivate *p);
        virtual bool event(QEvent *e);
        QDeclarativeThreadPrivate *p;
    };
    MainObject m_mainObject;

    QMutex _mutex;
    QWaitCondition _wait;
};

QDeclarativeThreadPrivate::MainObject::MainObject(QDeclarativeThreadPrivate *p) 
: p(p) 
{
}

// Trigger mainEvent in main thread.  Must be called from thread.
void QDeclarativeThreadPrivate::triggerMainEvent()
{
    Q_ASSERT(q->isThisThread());
    QCoreApplication::postEvent(&m_mainObject, new QEvent(QEvent::User));
}

// Trigger even in thread.  Must be called from main thread.
void QDeclarativeThreadPrivate::triggerThreadEvent()
{
    Q_ASSERT(!q->isThisThread());
    QCoreApplication::postEvent(this, new QEvent(QEvent::User));
}

bool QDeclarativeThreadPrivate::MainObject::event(QEvent *e) 
{
    if (e->type() == QEvent::User) 
        p->mainEvent();
    return QObject::event(e);
}
    
QDeclarativeThreadPrivate::QDeclarativeThreadPrivate(QDeclarativeThread *q)
: q(q), m_threadProcessing(false), m_mainProcessing(false), m_shutdown(false), 
  m_mainThreadWaiting(false), mainSync(0), m_mainObject(this)
{
}

bool QDeclarativeThreadPrivate::event(QEvent *e)
{
    if (e->type() == QEvent::User) 
        threadEvent();
    return QThread::event(e);
}

void QDeclarativeThreadPrivate::run()
{
    lock();

    wakeOne();

    unlock();

    q->startupThread();
    exec();
}

void QDeclarativeThreadPrivate::mainEvent()
{
    lock();

    m_mainProcessing = true;

    while (!mainList.isEmpty() || mainSync) {
        bool isSync = mainSync != 0;
        QDeclarativeThread::Message *message = isSync?mainSync:mainList.takeFirst();
        unlock();

        message->call(q);
        delete message;

        lock();

        if (isSync) {
            mainSync = 0;
            wakeOne();
        }
    }

    m_mainProcessing = false;

    unlock();
}

void QDeclarativeThreadPrivate::threadEvent() 
{
    lock();

    if (m_shutdown) {
        quit();
        wakeOne();
        unlock();
        q->shutdownThread();
    } else {
        m_threadProcessing = true;

        while (!threadList.isEmpty()) {
            QDeclarativeThread::Message *message = threadList.first();

            unlock();

            message->call(q);

            lock();

            delete threadList.takeFirst();
        }

        wakeOne();

        m_threadProcessing = false;

        unlock();
    }
}

QDeclarativeThread::QDeclarativeThread()
: d(new QDeclarativeThreadPrivate(this))
{
    d->lock();
    d->start();
    d->wait();
    d->unlock();
    d->moveToThread(d);

}

QDeclarativeThread::~QDeclarativeThread()
{
    delete d;
}

void QDeclarativeThread::shutdown()
{
    d->lock();
    Q_ASSERT(!d->m_shutdown);
    d->m_shutdown = true;
    if (d->threadList.isEmpty() && d->m_threadProcessing == false)
        d->triggerThreadEvent();
    d->wait();
    d->unlock();
    d->QThread::wait();
}

void QDeclarativeThread::lock()
{
    d->lock();
}

void QDeclarativeThread::unlock()
{
    d->unlock();
}

void QDeclarativeThread::wakeOne()
{
    d->wakeOne();
}

void QDeclarativeThread::wakeAll()
{
    d->wakeAll();
}

void QDeclarativeThread::wait()
{
    d->wait();
}

bool QDeclarativeThread::isThisThread() const
{
    return QThread::currentThread() == d;
}

QThread *QDeclarativeThread::thread() const
{
    return const_cast<QThread *>(static_cast<const QThread *>(d));
}

// Called when the thread starts.  Do startup stuff in here.
void QDeclarativeThread::startupThread()
{
}

// Called when the thread shuts down.  Do cleanup in here.
void QDeclarativeThread::shutdownThread()
{
}

void QDeclarativeThread::internalCallMethodInThread(Message *message)
{
    Q_ASSERT(!isThisThread());
    d->lock();
    Q_ASSERT(d->m_mainThreadWaiting == false);

    bool wasEmpty = d->threadList.isEmpty();
    d->threadList.append(message);
    if (wasEmpty && d->m_threadProcessing == false)
        d->triggerThreadEvent();

    d->m_mainThreadWaiting = true;

    do {
        if (d->mainSync) {
            QDeclarativeThread::Message *message = d->mainSync;
            unlock();
            message->call(this);
            delete message;
            lock();
            d->mainSync = 0;
            wakeOne();
        } else {
            d->wait();
        }
    } while (d->mainSync || !d->threadList.isEmpty());

    d->m_mainThreadWaiting = false;
    d->unlock();
}

void QDeclarativeThread::internalCallMethodInMain(Message *message)
{
    Q_ASSERT(isThisThread());

    d->lock();

    Q_ASSERT(d->mainSync == 0);
    d->mainSync = message;

    if (d->m_mainThreadWaiting) {
        d->wakeOne();
    } else if (d->m_mainProcessing) {
        // Do nothing - it is already looping
    } else {
        d->triggerMainEvent();
    }

    while (d->mainSync && !d->m_shutdown)
        d->wait();

    d->unlock();
}

void QDeclarativeThread::internalPostMethodToThread(Message *message)
{
    Q_ASSERT(!isThisThread());
    d->lock();
    bool wasEmpty = d->threadList.isEmpty();
    d->threadList.append(message);
    if (wasEmpty && d->m_threadProcessing == false)
        d->triggerThreadEvent();
    d->unlock();
}

void QDeclarativeThread::internalPostMethodToMain(Message *message)
{
    Q_ASSERT(isThisThread());
    d->lock();
    bool wasEmpty = d->mainList.isEmpty();
    d->mainList.append(message);
    if (wasEmpty && d->m_mainProcessing == false)
        d->triggerMainEvent();
    d->unlock();
}

QT_END_NAMESPACE
