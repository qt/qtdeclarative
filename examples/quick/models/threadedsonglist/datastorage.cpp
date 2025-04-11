// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "datastorage.h"
#include "queueworker.h"

DataStorage::DataStorage() : QObject{ nullptr }
{
    /* Generate a list of IDs to use locally.
     * DataStorage will use indexes for accessing the remote data.
     */
    m_idList.reserve(RemoteMedia::count());
    for (int id = 1; id <= RemoteMedia::count(); ++id) {
        m_idList.append(id);
    }
    m_worker = new QueueWorker;
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_worker->moveToThread(&m_workerThread);
    connect(m_worker, &QueueWorker::dataFetched, this, &DataStorage::dataReceived);
    connect(m_worker, &QueueWorker::processing, this, &DataStorage::fetchStarted);
    connect(m_worker, &QueueWorker::dropped, this, &DataStorage::fetchAborted);
    connect(this, &DataStorage::dataFetchNeeded, m_worker, &QueueWorker::fetchData);
    m_workerThread.start();
}

DataStorage::~DataStorage()
{
    /* Clean the worker object by stopping it and then ending the thread */
    m_worker->abort();
    m_worker = nullptr;
    m_workerThread.quit();
    m_workerThread.wait();
}

QList<int> DataStorage::idList()
{
    return m_idList;
}

MediaElement DataStorage::item(int id) const
{
    if (id < 1)
        return MediaElement{};
    //! [Send signal if no data]
    if (!m_items.contains(id)) {
        m_items.insert(id, MediaElement{});
        emit dataFetchNeeded(m_idList.indexOf(id));
    }
    return m_items.value(id);
    //! [Send signal if no data]
}

std::optional<int> DataStorage::currentlyFetchedId() const
{
    return m_currentlyFetchedId;
}

void DataStorage::fetchStarted(int index)
{
    const auto idBeingFetched = m_idList.at(index);
    m_currentlyFetchedId = idBeingFetched;
    emit dataUpdated(idBeingFetched);
}

void DataStorage::fetchAborted(int index)
{
    /* The data will never be fetched. Remove the empty item (so storge will send a signal to thread
     * if needed). Then send dataUpdated signal. If the item is still visible, view will re-request
     * its data and item will be requeued to thread. If item is no longer in visible area, view will
     * ignore the dataChanged signal and storage will not receive any calls.
     */
    const auto idAborted = m_idList.at(index);
    m_items.remove(idAborted);
    if (m_currentlyFetchedId == idAborted)
        m_currentlyFetchedId = std::nullopt;
    emit dataUpdated(idAborted);
}

void DataStorage::dataReceived(int index, MediaElement element)
{
    const auto idUpdated = m_idList.at(index);
    m_items.insert(idUpdated, element);
    emit dataUpdated(idUpdated);
}
