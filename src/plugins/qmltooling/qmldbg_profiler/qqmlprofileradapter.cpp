/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlprofileradapter.h"
#include "qqmldebugpacket.h"

#include <private/qqmldebugserviceinterfaces_p.h>

QT_BEGIN_NAMESPACE

QQmlProfilerAdapter::QQmlProfilerAdapter(QQmlProfilerService *service, QQmlEnginePrivate *engine) :
    next(0)
{
    setService(service);
    engine->enableProfiler();
    connect(this, SIGNAL(profilingEnabled(quint64)), engine->profiler, SLOT(startProfiling(quint64)));
    connect(this, SIGNAL(profilingEnabledWhileWaiting(quint64)),
            engine->profiler, SLOT(startProfiling(quint64)), Qt::DirectConnection);
    connect(this, SIGNAL(profilingDisabled()), engine->profiler, SLOT(stopProfiling()));
    connect(this, SIGNAL(profilingDisabledWhileWaiting()),
            engine->profiler, SLOT(stopProfiling()), Qt::DirectConnection);
    connect(this, SIGNAL(dataRequested()), engine->profiler, SLOT(reportData()));
    connect(this, SIGNAL(referenceTimeKnown(QElapsedTimer)),
            engine->profiler, SLOT(setTimer(QElapsedTimer)));
    connect(engine->profiler,
            SIGNAL(dataReady(QVector<QQmlProfilerData>,QQmlProfiler::LocationHash)),
            this,
            SLOT(receiveData(QVector<QQmlProfilerData>,QQmlProfiler::LocationHash)));
}

// convert to QByteArrays that can be sent to the debug client
// use of QDataStream can skew results
//     (see tst_qqmldebugtrace::trace() benchmark)
static void qQmlProfilerDataToByteArrays(const QQmlProfilerData &d,
                                         const QQmlProfiler::LocationHash &locations,
                                         QList<QByteArray> &messages)
{
    QQmlDebugPacket ds;
    Q_ASSERT_X((d.messageType & (1 << 31)) == 0, Q_FUNC_INFO,
               "You can use at most 31 message types.");
    for (quint32 decodedMessageType = 0; (d.messageType >> decodedMessageType) != 0;
         ++decodedMessageType) {
        if ((d.messageType & (1 << decodedMessageType)) == 0)
            continue;

        //### using QDataStream is relatively expensive
        ds << d.time << decodedMessageType << static_cast<quint32>(d.detailType);

        QQmlProfiler::Location l = locations.value(d.locationId);

        switch (decodedMessageType) {
        case QQmlProfilerDefinitions::RangeStart:
        case QQmlProfilerDefinitions::RangeEnd:
            break;
        case QQmlProfilerDefinitions::RangeData:
            ds << (l.location.sourceFile.isEmpty() ? l.url.toString() : l.location.sourceFile);
            break;
        case QQmlProfilerDefinitions::RangeLocation:
            ds << (l.url.isEmpty() ? l.location.sourceFile : l.url.toString())
               << static_cast<qint32>(l.location.line) << static_cast<qint32>(l.location.column);
            break;
        default:
            Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid message type.");
            break;
        }
        messages.append(ds.squeezedData());
        ds.clear();
    }
}

qint64 QQmlProfilerAdapter::sendMessages(qint64 until, QList<QByteArray> &messages)
{
    while (next != data.length()) {
        const QQmlProfilerData &nextData = data.at(next);
        if (nextData.time > until || messages.length() > s_numMessagesPerBatch)
            return nextData.time;
        qQmlProfilerDataToByteArrays(nextData, locations, messages);
        ++next;
    }

    next = 0;
    data.clear();
    locations.clear();
    return -1;
}

void QQmlProfilerAdapter::receiveData(const QVector<QQmlProfilerData> &new_data,
                                      const QQmlProfiler::LocationHash &new_locations)
{
    if (data.isEmpty())
        data = new_data;
    else
        data.append(new_data);

    if (locations.isEmpty())
        locations = new_locations;
    else
        locations.unite(new_locations);

    service->dataReady(this);
}

QT_END_NAMESPACE
