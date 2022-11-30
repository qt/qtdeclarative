// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TESTMODEL_H
#define TESTMODEL_H

#include <QtCore/qabstractitemmodel.h>
#include <QtQuick/qquickview.h>

class TreeItem
{
public:
    explicit TreeItem(TreeItem *parent);
    explicit TreeItem(int rootRow);
    ~TreeItem();

    int row() const;
    QVector<TreeItem *> m_childItems;
    TreeItem *m_parentItem;
    QVector<QVariant> m_entries;
    int rootRow = -1;
};

// ########################################################

class TestModel : public QAbstractItemModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit TestModel(QObject *parent = nullptr);

    void createTreeRecursive(TreeItem *parentItem, int childCount, int currentDepth, int maxDepth, const QString &branchTag);
    TreeItem *treeItem(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    Q_INVOKABLE bool insertRows(int position, int count, const QModelIndex &parent) override;
    Q_INVOKABLE bool removeRows(int position, int count, const QModelIndex &parent) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    QVector<TreeItem *> m_rootItems;
    int m_columnCount = 2;
};

#endif // TESTMODEL_H
