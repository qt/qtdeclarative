// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "testmodel.h"

TreeItem::TreeItem(TreeItem *parent)
    : m_parentItem(parent)
{}

TreeItem::~TreeItem()
{
    qDeleteAll(m_childItems);
}

int TreeItem::row() const
{
    if (!m_parentItem)
        return 0;
    return m_parentItem->m_childItems.indexOf(const_cast<TreeItem *>(this));
}

TestModel::TestModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_rootItem.reset(new TreeItem());
    for (int col = 0; col < m_columnCount; ++col)
        m_rootItem.data()->m_entries << QVariant(QString("0, %1").arg(col));
    createTreeRecursive(m_rootItem.data(), 4, 1);
}

void TestModel::createTreeRecursive(TreeItem *item, int childCount, int currentDepth)
{
    if (currentDepth > maxDepth())
        return;

    for (int row = 0; row < childCount; ++row) {
        auto childItem = new TreeItem(item);
        for (int col = 0; col < m_columnCount; ++col)
            childItem->m_entries << QVariant(QString("%1, %2").arg(row).arg(col));
        item->m_childItems.append(childItem);
        if (row == childCount - 2 && currentDepth != maxDepth()) {
            // Add a branch that doesn't recurse
            createTreeRecursive(childItem, childCount, maxDepth());
        }
        if (row == childCount - 1) {
            // Add a branch that recurses
            createTreeRecursive(childItem, childCount, currentDepth + 1);
        }
    }
}

TreeItem *TestModel::treeItem(const QModelIndex &index) const
{
    if (!index.isValid())
        return m_rootItem.data();
    return static_cast<TreeItem *>(index.internalPointer());
}

int TestModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 1; // root of the tree
    return treeItem(parent)->m_childItems.size();
}

int TestModel::columnCount(const QModelIndex &) const
{
    return m_columnCount;
}

QVariant TestModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role)
    if (!index.isValid())
        return QVariant();
    TreeItem *item = treeItem(index);
    return item->m_entries.at(index.column());
}

bool TestModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(role)
    if (!index.isValid())
        return false;
    TreeItem *item = treeItem(index);
    if (!item)
        return false;
    item->m_entries[index.column()] = value;
    emit dataChanged(index, index);
    return true;
}

QModelIndex TestModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(column)
    if (!hasIndex(row, column, parent))
        return QModelIndex();
    if (!parent.isValid())
        return createIndex(row, column, m_rootItem.data());
    return createIndex(row, column, treeItem(parent)->m_childItems.at(row));
}

QModelIndex TestModel::parent(const QModelIndex &index) const
{
    TreeItem *parentItem = treeItem(index)->m_parentItem;
    if (!parentItem)
        return QModelIndex();
    return createIndex(parentItem->row(), 0, parentItem);
}

bool TestModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    if (!parent.isValid()) {
        qWarning() << "Cannot insert rows on an invalid parent!";
        return false;
    }

    beginInsertRows(parent, position, position + rows - 1);
    TreeItem *parentItem = treeItem(parent);

    for (int row = 0; row < rows; ++row) {
        auto newChildItem = new TreeItem(parentItem);
        for (int col = 0; col < m_columnCount; ++col)
            newChildItem->m_entries << QVariant(QString("%1, %2 (inserted)").arg(position + row).arg(col));
        parentItem->m_childItems.insert(position + row, newChildItem);
    }

    endInsertRows();
    return true;
}
