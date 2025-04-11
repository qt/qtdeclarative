// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "threadedlistmodel.h"

ThreadedListModel::ThreadedListModel()
{
    m_idList = m_storage.idList();
    connect(&m_storage, &DataStorage::dataUpdated, this, &ThreadedListModel::dataUpdated);
}

int ThreadedListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_idList.size();
}

QVariant ThreadedListModel::data(const QModelIndex &index, int role) const
{
    const auto id = m_idList.at(index.row());
    const auto &item = m_storage.item(id);
    if (item.isValid()) {
        switch (static_cast<Role>(role)) {
        case Role::Album:
            return item.album();
        case Role::AlbumArt:
            return item.albumArtFile();
        case Role::Artist:
            return item.artist();
        case Role::Song:
            return item.song();
        case Role::LoadingElement:
            return false;
        case Role::LoadingText:
            return QString();
        default:
            break;
        }
    } else {
        /* Item not fetched yet, pass "loading" item. */
        switch (static_cast<Role>(role)) {
        case Role::Album:
        case Role::AlbumArt:
        case Role::Artist:
        case Role::Song:
            return QString{};
        case Role::LoadingElement:
            return true;
        case Role::LoadingText:
            return m_storage.currentlyFetchedId() == id ? QString{} : tr("Waiting...");
        default:
            break;
        }
    }
    return QVariant{};
}

void ThreadedListModel::dataUpdated(int id)
{
    const QModelIndex changedIndex = index(m_idList.indexOf(id));
    emit dataChanged(changedIndex, changedIndex);
}

QHash<int, QByteArray> ThreadedListModel::roleNames() const
{
    static const QHash<int, QByteArray> roles{{static_cast<int>(Role::Song), "song"},
                                              {static_cast<int>(Role::Artist), "artist"},
                                              {static_cast<int>(Role::Album), "album"},
                                              {static_cast<int>(Role::AlbumArt), "albumArt"},
                                              {static_cast<int>(Role::LoadingText), "loadingText"},
                                              {static_cast<int>(Role::LoadingElement),
                                               "isLoadingElement"}};
    return roles;
}
