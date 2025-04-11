// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "queueworker.h"

/* view needs 8 items and cacheBuffer of 2 items above and below to show visible area, so limit queue max length to that. */
static constexpr int s_queueMaxLength{12};

QueueWorker::QueueWorker()
    : QObject{nullptr}
{
    connect(this,
            &QueueWorker::triggerQueueProcessing,
            this,
            &QueueWorker::processQueue,
            Qt::QueuedConnection);
}

void QueueWorker::abort()
{
    m_killSignalReceived = true;
}

void QueueWorker::fetchData(int id)
{
    if (m_indicesAlreadyFetched.contains(id) || m_indicesToFetch.contains(id)
        || m_killSignalReceived)
        return;

    /* This is signal from UI thread. It is likely that there are more than one in queue.
     * Add our own triggerQueueProcessing() signal to the end of event queue and add id to queue of IDs.
     * Then let all queued signals to be processed until QueueWorker receives its own signal.
     */
    sendTriggerQueueProcessingSignal(id);
}

void QueueWorker::processQueue()
{
    /* This slot receives QueueWorker's own signal after all signals from UI thread have been processed. */
    m_queueProcessingSignalSent = false;
    const auto index = m_indicesToFetch.dequeue();
    emit processing(index);
    const auto data = m_remoteMedia.getElements(1, index);
    m_indicesAlreadyFetched.insert(index);
    emit dataFetched(index, data.front());
    if (!m_indicesToFetch.empty() && !m_killSignalReceived) {
        /* Queueworker should process more indices, but the UI thread may have sent more signals during previous processing.
         * Add our own signal to end of event queue and let all preceding signals to be processed before continuing.
         */
        emit triggerQueueProcessing(QPrivateSignal{});
        m_queueProcessingSignalSent = true;
    }
    /* Else all fetches have been processed so far. */
}

void QueueWorker::sendTriggerQueueProcessingSignal(int id)
{
    /* Make sure excess IDs are removed. */
    while (m_indicesToFetch.size() >= s_queueMaxLength) {
        emit dropped(m_indicesToFetch.dequeue());
    }
    /* Before calling this function QueueWorker has ensured that m_indicesToFetch doesn't contain id yet, so just add id. */
    m_indicesToFetch.enqueue(id);
    if (!m_queueProcessingSignalSent) {
        emit triggerQueueProcessing(QPrivateSignal{});
        m_queueProcessingSignalSent = true;
    }
}
