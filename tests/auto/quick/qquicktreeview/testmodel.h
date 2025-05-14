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

    void createTreeRecursive(TreeItem *item, int childCount, int currentDepth);
    TreeItem *treeItem(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    int maxDepth() { return 4; }

    bool insertRows(int position, int rows, const QModelIndex &parent) override;
    bool insertColumns(int position, int cols, const QModelIndex &parent) override;

    QList<QModelIndex> m_editableIndices;

private:
    QScopedPointer<TreeItem> m_rootItem;
    int m_columnCount = 5;
};

#define TestModelAsVariant(...) QVariant::fromValue(QSharedPointer<TestModel>(new TestModel(__VA_ARGS__)))

#endif // TESTMODEL_H
