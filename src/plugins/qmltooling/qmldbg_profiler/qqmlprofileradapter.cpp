// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlprofileradapter.h"
#include "qqmlprofilerservice.h"

#include <private/qqmldebugserviceinterfaces_p.h>

QT_BEGIN_NAMESPACE

QQmlProfilerAdapter::QQmlProfilerAdapter(QQmlProfilerService *service, QQmlEnginePrivate *engine)
{
    engine->profiler = new QQmlProfiler;
    init(service, engine->profiler);
}

QQmlProfilerAdapter::QQmlProfilerAdapter(QQmlProfilerService *service, QQmlTypeLoader *loader)
{
    QQmlProfiler *profiler = new QQmlProfiler;
    loader->setProfiler(profiler);
    init(service, profiler);
}

void QQmlProfilerAdapter::init(QQmlProfilerService *service, QQmlProfiler *profiler)
{
    next = 0;
    setService(service);
    connect(this, &QQmlProfilerAdapter::profilingEnabled,
            profiler, &QQmlProfiler::startProfiling);
    connect(this, &QQmlAbstractProfilerAdapter::profilingEnabledWhileWaiting,
            profiler, &QQmlProfiler::startProfiling, Qt::DirectConnection);
    connect(this, &QQmlAbstractProfilerAdapter::profilingDisabled,
            profiler, &QQmlProfiler::stopProfiling);
    connect(this, &QQmlAbstractProfilerAdapter::profilingDisabledWhileWaiting,
            profiler, &QQmlProfiler::stopProfiling, Qt::DirectConnection);
    connect(this, &QQmlAbstractProfilerAdapter::dataRequested,
            profiler, &QQmlProfiler::reportData);
    connect(this, &QQmlAbstractProfilerAdapter::referenceTimeKnown,
            profiler, &QQmlProfiler::setTimer);
    connect(profiler, &QQmlProfiler::dataReady,
            this, &QQmlProfilerAdapter::receiveData);
}

// convert to QByteArrays that can be sent to the debug client
static void qQmlProfilerDataToByteArrays(const QQmlProfilerData &d,
                                         QQmlProfiler::LocationHash &locations,
                                         QList<QByteArray> &messages)
{
    QQmlDebugPacket ds;
    Q_ASSERT_X((d.messageType & (1 << 31)) == 0, Q_FUNC_INFO,
               "You can use at most 31 message types.");
    for (quint32 decodedMessageType = 0; (d.messageType >> decodedMessageType) != 0;
         ++decodedMessageType) {
        if (decodedMessageType == QQmlProfilerDefinitions::RangeData
                || (d.messageType & (1 << decodedMessageType)) == 0) {
            continue; // RangeData is sent together with RangeLocation
        }

        if (decodedMessageType == QQmlProfilerDefinitions::RangeEnd
                || decodedMessageType == QQmlProfilerDefinitions::RangeStart) {
            ds << d.time << decodedMessageType << static_cast<quint32>(d.detailType);
            if (d.locationId != 0)
                ds << static_cast<qint64>(d.locationId);
        } else {
            auto i = locations.constFind(d.locationId);
            if (i != locations.cend()) {
                ds << d.time << decodedMessageType << static_cast<quint32>(d.detailType);
                ds << (i->url.isEmpty() ? i->location.sourceFile : i->url.toString())
                   << static_cast<qint32>(i->location.line)
                   << static_cast<qint32>(i->location.column);
                if (d.messageType & (1 << QQmlProfilerDefinitions::RangeData)) {
                    // Send both, location and data ...
                    ds << static_cast<qint64>(d.locationId);
                    messages.append(ds.squeezedData());
                    ds.clear();
                    ds << d.time << int(QQmlProfilerDefinitions::RangeData)
                       << static_cast<quint32>(d.detailType)
                       << (i->location.sourceFile.isEmpty() ? i->url.toString() :
                                                              i->location.sourceFile);
                }
                ds << static_cast<qint64>(d.locationId);
                locations.erase(i); // ... so that we can erase here without missing anything.
            } else {
                // Skip RangeData and RangeLocation: We've already sent them
                continue;
            }
        }
        messages.append(ds.squeezedData());
        ds.clear();
    }
}

qint64 QQmlProfilerAdapter::sendMessages(qint64 until, QList<QByteArray> &messages)
{
    while (next != data.size()) {
        const QQmlProfilerData &nextData = data.at(next);
        if (nextData.time > until || messages.size() > s_numMessagesPerBatch)
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
        locations.insert(new_locations);

    service->dataReady(this);
}

QT_END_NAMESPACE

#include "moc_qqmlprofileradapter.cpp"
