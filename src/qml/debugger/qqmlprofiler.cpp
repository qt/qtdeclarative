// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlprofiler_p.h"
#include "qqmldebugservice_p.h"

QT_BEGIN_NAMESPACE

QQmlProfiler::QQmlProfiler() : featuresEnabled(0)
{
    static int metatype = qRegisterMetaType<QVector<QQmlProfilerData> >();
    static int metatype2 = qRegisterMetaType<QQmlProfiler::LocationHash> ();
    Q_UNUSED(metatype);
    Q_UNUSED(metatype2);
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
    m_locations.clear();
}

void QQmlProfiler::reportData()
{
    LocationHash resolved;
    resolved.reserve(m_locations.size());
    for (auto it = m_locations.begin(), end = m_locations.end(); it != end; ++it) {
        if (!it->sent) {
            resolved.insert(it.key(), it.value());
            it->sent = true;
        }
    }

    QVector<QQmlProfilerData> data;
    data.swap(m_data);
    emit dataReady(data, resolved);
}

QT_END_NAMESPACE

#include "moc_qqmlprofiler_p.cpp"
