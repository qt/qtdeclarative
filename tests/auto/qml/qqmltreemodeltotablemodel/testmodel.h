// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TESTMODEL_H
#define TESTMODEL_H

#include <QtCore/qabstractitemmodel.h>
#include <QtQuick/qquickview.h>

class TreeItem
{
public:
    explicit TreeItem(TreeItem *parent = nullptr);
    ~TreeItem();

    int row() const;
    QVector<TreeItem *> m_childItems;
    TreeItem *m_parentItem;
    QVector<QVariant> m_entries;
};

// ########################################################

class TestModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit TestModel(QObject *parent = nullptr);

    void createTreeRecursive(TreeItem *item, int childCount, int currentDepth, int maxDepth);
    TreeItem *treeItem(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    bool insertRows(int position, int rows, const QModelIndex &parent) override;
    bool insertColumns(int position, int rows, const QModelIndex &parent) override;

private:
    QScopedPointer<TreeItem> m_rootItem;
    int m_columnCount = 2;
};

#endif // TESTMODEL_H
