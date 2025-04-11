// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DATASTORAGE_H
#define DATASTORAGE_H

#include "mediaelement.h"

#include <QHash>
#include <QThread>

class QueueWorker;

class DataStorage : public QObject
{
    Q_OBJECT
public:
    explicit DataStorage();
    ~DataStorage();

    //! [Interface towards the model]
    QList<int> idList();
    MediaElement item(int id) const;
    std::optional<int> currentlyFetchedId() const;
    //! [Interface towards the model]

signals:
    // Towards client
    void dataUpdated(int id);
    // Towards worker
    void dataFetchNeeded(int index) const;

private slots:
    // From worker
    void fetchStarted(int index);
    void fetchAborted(int index);
    void dataReceived(int index, MediaElement element);

private:
    QueueWorker *m_worker;
    QList<int> m_idList;
    QThread m_workerThread;
    std::optional<int> m_currentlyFetchedId;
    /* The item list is mutable as DataStorage needs to mark already signaled items to it in
     * const data() call of model */
    mutable QHash<int, MediaElement> m_items;
};

#endif // DATASTORAGE_H
