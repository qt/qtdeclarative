// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef THREADEDLISTMODEL_H
#define THREADEDLISTMODEL_H

#include "datastorage.h"

#include <QAbstractListModel>
#include <QQmlEngine>

class ThreadedListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit ThreadedListModel();
    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_INVOKABLE QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

protected:
    enum class Role {
        Song = Qt::ItemDataRole::UserRole,
        Artist,
        Album,
        AlbumArt,
        LoadingElement,
        LoadingText
    };

private slots:
    void dataUpdated(int id);

private:
    DataStorage m_storage;
    QList<int> m_idList;
    int m_currentSongId{-1};
};

#endif // THREADEDLISTMODEL_H
