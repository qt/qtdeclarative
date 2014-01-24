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

#include "qqmlprofiler_p.h"
#include "qqmlprofilerservice_p.h"
#include "qqmldebugservice_p.h"

QT_BEGIN_NAMESPACE

// convert to QByteArrays that can be sent to the debug client
// use of QDataStream can skew results
//     (see tst_qqmldebugtrace::trace() benchmark)
void QQmlProfilerData::toByteArrays(QList<QByteArray> &messages) const
{
    QByteArray data;
    Q_ASSERT_X(((messageType | detailType) & (1 << 31)) == 0, Q_FUNC_INFO, "You can use at most 31 message types and 31 detail types.");
    for (uint decodedMessageType = 0; (messageType >> decodedMessageType) != 0; ++decodedMessageType) {
        if ((messageType & (1 << decodedMessageType)) == 0)
            continue;

        for (uint decodedDetailType = 0; (detailType >> decodedDetailType) != 0; ++decodedDetailType) {
            if ((detailType & (1 << decodedDetailType)) == 0)
                continue;

            //### using QDataStream is relatively expensive
            QQmlDebugStream ds(&data, QIODevice::WriteOnly);
            ds << time << decodedMessageType << decodedDetailType;

            switch (decodedMessageType) {
            case QQmlProfilerDefinitions::RangeStart:
                if (decodedDetailType == (int)QQmlProfilerDefinitions::Binding)
                    ds << QQmlProfilerDefinitions::QmlBinding;
                break;
            case QQmlProfilerDefinitions::RangeData:
                ds << detailString;
                break;
            case QQmlProfilerDefinitions::RangeLocation:
                ds << (detailUrl.isEmpty() ? detailString : detailUrl.toString()) << x << y;
                break;
            case QQmlProfilerDefinitions::RangeEnd: break;
            default:
                Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid message type.");
                break;
            }
            messages << data;
            data.clear();
        }
    }
}

QQmlProfilerAdapter::QQmlProfilerAdapter(QQmlProfilerService *service, QQmlEnginePrivate *engine) :
    QQmlAbstractProfilerAdapter(service)
{
    engine->enableProfiler();
    connect(this, SIGNAL(profilingEnabled()), engine->profiler, SLOT(startProfiling()));
    connect(this, SIGNAL(profilingEnabledWhileWaiting()),
            engine->profiler, SLOT(startProfiling()), Qt::DirectConnection);
    connect(this, SIGNAL(profilingDisabled()), engine->profiler, SLOT(stopProfiling()));
    connect(this, SIGNAL(profilingDisabledWhileWaiting()),
            engine->profiler, SLOT(stopProfiling()), Qt::DirectConnection);
    connect(this, SIGNAL(dataRequested()), engine->profiler, SLOT(reportData()));
    connect(this, SIGNAL(referenceTimeKnown(const QElapsedTimer &)),
            engine->profiler, SLOT(setTimer(const QElapsedTimer &)));
    connect(engine->profiler, SIGNAL(dataReady(const QList<QQmlProfilerData> &)),
            this, SLOT(receiveData(const QList<QQmlProfilerData> &)));
}

qint64 QQmlProfilerAdapter::sendMessages(qint64 until, QList<QByteArray> &messages)
{
    while (!data.empty() && data.front().time <= until) {
        data.front().toByteArrays(messages);
        data.pop_front();
    }
    return data.empty() ? -1 : data.front().time;
}

void QQmlProfilerAdapter::receiveData(const QList<QQmlProfilerData> &new_data)
{
    data = new_data;
    service->dataReady(this);
}


QQmlProfiler::QQmlProfiler() : enabled(false)
{
    static int metatype = qRegisterMetaType<QList<QQmlProfilerData> >();
    Q_UNUSED(metatype);
    m_timer.start();
}

void QQmlProfiler::startProfiling()
{
    enabled = true;
}

void QQmlProfiler::stopProfiling()
{
    enabled = false;
    reportData();
    m_data.clear();
}

void QQmlProfiler::reportData()
{
    QList<QQmlProfilerData> result;
    result.reserve(m_data.size());
    for (int i = 0; i < m_data.size(); ++i)
        result.append(m_data[i]);
    emit dataReady(result);
}

/*!
 * \fn void QQmlVmeProfiler::Data::clear()
 * Resets the profiling data to defaults.
 */

/*!
 * \fn bool QQmlVmeProfiler::startBackground(const QString &typeName)
 * If profiling is enabled clears the current range data, then stops the
 * profiler previously running in the foreground if any, then starts a new one
 * in the background, setting the given typeName. \a typeName is the type of
 * object being created.
 */

/*!
 * \fn bool QQmlVmeProfiler::start(const QString &typeName, const QUrl &url, int line, int column)
 * If profiling is enabled clears the current range data, then stops the
 * profiler previously running in the foreground if any, then starts a new one
 * in the foreground, setting the given location. \a url is the URL of
 * file being executed, \line line is the current line in in that file, and
 * \a column is the current column in that file.
 */

/*!
 * \fn bool QQmlVmeProfiler::pop()
 * Stops the currently running profiler, if any, then retrieves an old one from the stack
 * of paused profilers and starts that if possible.
 */

/*!
 * \fn void QQmlVmeProfiler::push()
 * Pushes the currently running profiler on the stack of paused profilers. Note: The profiler
 * isn't paused here. That's a separate step. If it's never paused, but pop()'ed later that
 * won't do any harm, though.
 */

/*!
 * \fn void QQmlVmeProfiler::clear()
 * Stops the currently running (foreground and background) profilers and removes all saved
 * data about paused profilers.
 */

/*!
 * \fn void QQmlVmeProfiler::stop()
 * Stop profiler running in the foreground, if any.
 */

/*!
 * \fn bool QQmlVmeProfiler::foreground(const QUrl &url, int line, int column)
 * Stops the profiler currently running in the foreground, if any and puts the
 * next profiler from the background in its place if there are any profilers in
 * the background. Additionally the rangeLocation is set. \a url is the URL of
 * file being executed, \line line is the current line in in that file, and
 * \a column is the current column in that file.
 */

QT_END_NAMESPACE
