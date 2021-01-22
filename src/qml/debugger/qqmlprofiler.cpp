/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
