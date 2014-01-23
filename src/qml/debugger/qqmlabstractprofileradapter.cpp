/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qqmlabstractprofileradapter_p.h"

QT_BEGIN_NAMESPACE

/*!
 * \class QQmlAbstractProfilerAdapter
 * Abstract base class for all adapters between profilers and the QQmlProfilerService. Adapters have
 * to retrieve profiler-specific data and convert it to the format sent over the wire. Adapters must
 * live in the QDebugServer thread but the actual profilers can live in different threads. The
 * recommended way to deal with this is passing the profiling data through a signal/slot connection.
 */

/*!
 * \fn void QQmlAbstractProfilerAdapter::dataReady(QQmlAbstractProfilerAdapter *)
 * Signals that data has been extracted from the profiler and is readily available in the adapter.
 * The primary data representation is in satellite's format. It should be transformed and deleted
 * on the fly with sendMessages.
 */

/*!
 * \fn void QQmlAbstractProfilerAdapter::dataRequested()
 * Signals that data has been requested by the \c QQmlProfilerService. This signal should be
 * connected to a slot in the profiler and the profiler should then transfer its currently available
 * profiling data to the adapter as soon as possible.
 */

/*!
 * \fn qint64 QQmlAbstractProfilerAdapter::sendMessages(qint64 until, QList<QByteArray> &messages)
 * Append the messages up to the timestamp \a until, chronologically sorted, to \a messages. Keep
 * track of the messages already sent and with each subsequent call to this method start with the
 * first one not yet sent. Messages that have been sent can be deleted. When new data from the
 * profiler arrives the information about the last sent message must be reset. Return the timestamp
 * of the next message after \a until or \c -1 if there is no such message.
 * The profiler service keeps a list of adapters, sorted by time of next message and keeps querying
 * the first one to send messages up to the time of the second one. Like that we get chronologically
 * sorted messages and can occasionally post the messages to exploit parallelism and save memory.
 */

/*!
 * \fn qint64 QQmlAbstractProfilerAdapter::startProfiling()
 * Emits either \c profilingEnabled() or \c profilingEnabledWhileWaiting(), depending on \c waiting.
 * If the profiler's thread is waiting for an initial start signal we can emit the signal over a
 * \c Qt::DirectConnection to avoid the delay of the event loop.
 */
void QQmlAbstractProfilerAdapter::startProfiling()
{
    if (waiting)
        emit profilingEnabledWhileWaiting();
    else
        emit profilingEnabled();
    running = true;
}

/*!
 * \fn qint64 QQmlAbstractProfilerAdapter::stopProfiling()
 * Emits either \c profilingDisabled() or \c profilingDisabledWhileWaiting(), depending on
 * \c waiting. If the profiler's thread is waiting for an initial start signal we can emit the
 * signal over a \c Qt::DirectConnection to avoid the delay of the event loop.
 */
void QQmlAbstractProfilerAdapter::stopProfiling() {
    if (waiting)
        emit profilingDisabledWhileWaiting();
    else
        emit profilingDisabled();
    running = false;
}

QT_END_NAMESPACE
