// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "testmodel.h"

TreeItem::TreeItem(TreeItem *parent)
    : m_parentItem(parent)
{}

TreeItem::TreeItem(int rootRow)
    : m_parentItem(nullptr)
    , rootRow(rootRow)
{}

TreeItem::~TreeItem()
{
    qDeleteAll(m_childItems);
}

int TreeItem::row() const
{
    if (!m_parentItem) {
        Q_ASSERT(rootRow != -1);
        return rootRow;
    }
    return m_parentItem->m_childItems.indexOf(const_cast<TreeItem *>(this));
}

TestModel::TestModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_rootItems.append(new TreeItem(0));
    m_rootItems.append(new TreeItem(1));
    m_rootItems.append(new TreeItem(2));

    for (int row = 0; row < m_rootItems.count(); ++row) {
        const QString branchTag = QString::number(row);
        for (int col = 0; col < m_columnCount; ++col)
            m_rootItems[row]->m_entries << QVariant(QString("r:%1, c:%2, d:0, b:%3").arg(row).arg(col).arg(branchTag));
        createTreeRecursive(m_rootItems[row], 4, 1, 5, branchTag);
    }
}

void TestModel::createTreeRecursive(TreeItem *parentItem, int childCount, int currentDepth, int maxDepth, const QString &branchTag)
{
    for (int row = 0; row < childCount; ++row) {
        auto childItem = new TreeItem(parentItem);

        if (currentDepth < maxDepth && row == 0)
            createTreeRecursive(childItem, childCount, currentDepth + 1, maxDepth, branchTag);

        for (int col = 0; col < m_columnCount; ++col)
            childItem->m_entries << QVariant(QString("r:%1, c:%2, d:%3, b:%4").arg(row).arg(col).arg(currentDepth).arg(branchTag));
        parentItem->m_childItems.append(childItem);
    }
}

TreeItem *TestModel::treeItem(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;
    return static_cast<TreeItem *>(index.internalPointer());
}

int TestModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_rootItems.count();
    return treeItem(parent)->m_childItems.count();
}

int TestModel::columnCount(const QModelIndex &) const
{
    return m_columnCount;
}

QVariant TestModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case Qt::SizeHintRole:
        return QSize(100, 20);
    case Qt::DisplayRole: {
        if (!index.isValid())
            return QVariant("invalid index");
        TreeItem *item = treeItem(index);
        return item->m_entries.at(index.column());
        break; }
    default:
        break;
    }
    return QVariant();
}

bool TestModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(role)
    if (!index.isValid())
        return false;
    TreeItem *item = treeItem(index);
    Q_ASSERT(item);
    item->m_entries[index.column()] = value;
    emit dataChanged(index, index);
    return true;
}

QModelIndex TestModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();
    if (!parent.isValid())
        return createIndex(row, column, m_rootItems[row]);
    return createIndex(row, column, treeItem(parent)->m_childItems.at(row));
}

QModelIndex TestModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *item = treeItem(index);
    TreeItem *parentItem = item->m_parentItem;
    if (!parentItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

bool TestModel::insertRows(int position, int count, const QModelIndex &parent)
{
    if (!parent.isValid()) {
        qWarning() << "Cannot insert rows on an invalid parent!";
        return false;
    }

    beginInsertRows(parent, position, position + count - 1);
    TreeItem *parentItem = treeItem(parent);

    for (int row = 0; row < count; ++row) {
        auto newChildItem = new TreeItem(parentItem);
        for (int col = 0; col < m_columnCount; ++col)
            newChildItem->m_entries << QVariant(QString("(inserted at %1, %2)").arg(position + row).arg(col));
        parentItem->m_childItems.insert(position + row, newChildItem);
    }

    endInsertRows();
    return true;
}

bool TestModel::removeRows(int position, int count, const QModelIndex &parent)
{
    if (!parent.isValid()) {
        qWarning() << "Cannot remove rows on an invalid parent!";
        return false;
    }

    beginRemoveRows(parent, position, position + count - 1);

    TreeItem *parentItem = treeItem(parent);
    parentItem->m_childItems.remove(position, count);

    endRemoveRows();
    return true;
}

Qt::ItemFlags TestModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}
