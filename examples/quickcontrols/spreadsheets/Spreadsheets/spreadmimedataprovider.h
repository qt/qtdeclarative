// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SPREADMIMEDATAPROVIDER_H
#define SPREADMIMEDATAPROVIDER_H

#include <QObject>
#include <QAbstractItemModel>
#include <QPoint>
#include <QQmlEngine>

class QMimeData;

class SpreadMimeDataProvider : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    QMimeData *saveToMimeData() const;
    bool loadFromMimeData(const QMimeData *mimeData);

    Q_INVOKABLE bool saveToClipboard();
    Q_INVOKABLE bool loadFromClipboard();
    Q_INVOKABLE void reset() { m_data.clear(); }
    Q_INVOKABLE int size() const { return m_data.size(); }
    Q_INVOKABLE QPoint cellAt(int index) const { return m_data.at(index).first; }
    Q_INVOKABLE bool saveDataToModel(int index,
                                     const QModelIndex &modelIndex,
                                     QAbstractItemModel *model) const;
    Q_INVOKABLE void loadDataFromModel(const QPoint &cell,
                                       const QModelIndex &index,
                                       const QAbstractItemModel *model);

private:
    std::vector<std::pair<QPoint, QMap<int, QVariant>>> m_data;
};

#endif // SPREADMIMEDATAPROVIDER_H
