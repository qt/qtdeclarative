// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QQMLPROFILERADAPTER_H
#define QQMLPROFILERADAPTER_H

#include <private/qqmlabstractprofileradapter_p.h>
#include <private/qqmlprofiler_p.h>

QT_BEGIN_NAMESPACE

class QQmlProfilerAdapter : public QQmlAbstractProfilerAdapter {
    Q_OBJECT
public:
    QQmlProfilerAdapter(QQmlProfilerService *service, QQmlEnginePrivate *engine);
    QQmlProfilerAdapter(QQmlProfilerService *service, QQmlTypeLoader *loader);
    qint64 sendMessages(qint64 until, QList<QByteArray> &messages) override;

    void receiveData(const QVector<QQmlProfilerData> &new_data,
                     const QQmlProfiler::LocationHash &locations);

private:
    void init(QQmlProfilerService *service, QQmlProfiler *profiler);
    QVector<QQmlProfilerData> data;
    QQmlProfiler::LocationHash locations;
    int next;
};

QT_END_NAMESPACE

#endif // QQMLPROFILERADAPTER_H
