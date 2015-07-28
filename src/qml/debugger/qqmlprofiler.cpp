/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    QQmlAbstractProfilerAdapter(service), next(0)
{
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
    connect(engine->profiler, SIGNAL(dataReady(QVector<QQmlProfilerData>)),
            this, SLOT(receiveData(QVector<QQmlProfilerData>)));
}

qint64 QQmlProfilerAdapter::sendMessages(qint64 until, QList<QByteArray> &messages)
{
    while (next != data.length()) {
        if (data[next].time > until)
            return data[next].time;
        data[next++].toByteArrays(messages);
    }

    next = 0;
    data.clear();
    return -1;
}

void QQmlProfilerAdapter::receiveData(const QVector<QQmlProfilerData> &new_data)
{
    if (data.isEmpty())
        data = new_data;
    else
        data.append(new_data);
    service->dataReady(this);
}


QQmlProfiler::QQmlProfiler() : featuresEnabled(0)
{
    static int metatype = qRegisterMetaType<QVector<QQmlProfilerData> >();
    Q_UNUSED(metatype);
    m_timer.start();
}

void QQmlProfiler::startProfiling(quint64 features)
{
    featuresEnabled = features;
}

void QQmlProfiler::stopProfiling()
{
    featuresEnabled = false;
    reportData();
}

void QQmlProfiler::reportData()
{
    emit dataReady(m_data);
    m_data.clear();
}

QT_END_NAMESPACE
