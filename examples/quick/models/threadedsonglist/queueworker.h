// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef QUEUEWORKER_H
#define QUEUEWORKER_H

#include "remotemedia.h"

#include <QObject>
#include <QQueue>
#include <QSet>

class QueueWorker : public QObject
{
    Q_OBJECT
public:
    explicit QueueWorker();
    void abort();

signals:
    void processing(int index);
    void dropped(int index);
    void dataFetched(int index, MediaElement element);
    void triggerQueueProcessing(QPrivateSignal); // signal to self, see implementation of fetchData()

public slots:
    void fetchData(int index);

private slots:
    void processQueue();

private:
    void sendTriggerQueueProcessingSignal(int index);
    RemoteMedia m_remoteMedia;
    QSet<int> m_indicesAlreadyFetched;
    QQueue<int> m_indicesToFetch;
    std::atomic<bool> m_killSignalReceived{false};
    bool m_queueProcessingSignalSent{false};
};

#endif // QUEUEWORKER_H
