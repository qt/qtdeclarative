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

#ifndef QV4PROFILERADAPTER_P_H
#define QV4PROFILERADAPTER_P_H

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

#include "qqmlprofilerservice.h"

#include <private/qv4profiling_p.h>
#include <private/qqmlabstractprofileradapter_p.h>

#include <QStack>
#include <QList>

QT_BEGIN_NAMESPACE

class QQmlProfilerService;
class QV4ProfilerAdapter : public QQmlAbstractProfilerAdapter {
    Q_OBJECT

public:
    QV4ProfilerAdapter(QQmlProfilerService *service, QV4::ExecutionEngine *engine);

    virtual qint64 sendMessages(qint64 until, QList<QByteArray> &messages) override;

    void receiveData(const QV4::Profiling::FunctionLocationHash &,
                     const QVector<QV4::Profiling::FunctionCallProperties> &,
                     const QVector<QV4::Profiling::MemoryAllocationProperties> &);

signals:
    void v4ProfilingEnabled(quint64 v4Features);
    void v4ProfilingEnabledWhileWaiting(quint64 v4Features);

private:
    QV4::Profiling::FunctionLocationHash m_functionLocations;
    QVector<QV4::Profiling::FunctionCallProperties> m_functionCallData;
    QVector<QV4::Profiling::MemoryAllocationProperties> m_memoryData;
    int m_functionCallPos;
    int m_memoryPos;
    QStack<qint64> m_stack;
    qint64 appendMemoryEvents(qint64 until, QList<QByteArray> &messages, QQmlDebugPacket &d);
    qint64 finalizeMessages(qint64 until, QList<QByteArray> &messages, qint64 callNext,
                            QQmlDebugPacket &d);
    void forwardEnabled(quint64 features);
    void forwardEnabledWhileWaiting(quint64 features);

    static quint64 translateFeatures(quint64 qmlFeatures);
};

QT_END_NAMESPACE

#endif // QV4PROFILERADAPTER_P_H
