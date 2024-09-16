// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef Q_TEST_MODEL_H
#define Q_TEST_MODEL_H

#include <QtCore/qabstractitemmodel.h>

#include <limits.h>

class TestModel: public QAbstractItemModel
{
    Q_OBJECT

public:
    TestModel(QObject *parent = nullptr): QAbstractItemModel(parent),
       fetched(false), rows(10), cols(1), levels(INT_MAX), wrongIndex(false) { init(); }

    TestModel(int _rows, int _cols, QObject *parent = nullptr): QAbstractItemModel(parent),
       fetched(false), rows(_rows), cols(_cols), levels(INT_MAX), wrongIndex(false) { init(); }

    ~TestModel()
    {
        delete tree;
    }

    void init() {
        decorationsEnabled = false;
        alternateChildlessRows = true;
        tree = new Node(rows);
    }

    inline qint32 level(const QModelIndex &index) const {
        Node *n = (Node *)index.internalPointer();
        if (!n)
            return -1;
        int l = -1;
        while (n != tree) {
            n = n->parent;
            ++l;
        }
        return l;
    }

    void resetModel()
    {
        beginResetModel();
        fetched = false;
        delete tree;
        tree = new Node(rows);
        endResetModel();
    }

    QString displayData(const QModelIndex &idx) const
    {
        return QString("[%1,%2,%3,%4]").arg(idx.row()).arg(idx.column()).arg(idx.internalId()).arg(hasChildren(idx));
    }

    bool canFetchMore(const QModelIndex &) const override {
        return !fetched;
    }

    void fetchMore(const QModelIndex &) override {
        fetched = true;
    }

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override {
        bool hasFetched = fetched;
        fetched = true;
        bool r = QAbstractItemModel::hasChildren(parent);
        fetched = hasFetched;
        return r;
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        if ((parent.column() > 0) || (level(parent) > levels)
            || (alternateChildlessRows && parent.row() > 0 && (parent.row() & 1)))
            return 0;
        Node *n = (Node*)parent.internalPointer();
        if (!n)
            n = tree;
        return n->children.size();
    }

    int columnCount(const QModelIndex& parent = QModelIndex()) const override {
        if ((parent.column() > 0) || (level(parent) > levels)
            || (alternateChildlessRows && parent.row() > 0 && (parent.row() & 1)))
            return 0;
        return cols;
    }

    bool isEditable(const QModelIndex &index) const {
        if (index.isValid())
            return true;
        return false;
    }

    Q_INVOKABLE QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        if (row < 0 || column < 0 || (level(parent) > levels) || column >= cols)
            return QModelIndex();
        Node *pn = (Node*)parent.internalPointer();
        if (!pn)
            pn = tree;
        if (row >= pn->children.size())
            return QModelIndex();

        Node *n = pn->children.at(row);
        if (!n) {
            n = new Node(rows, pn);
            pn->children[row] = n;
        }
        return createIndex(row, column, n);
    }

    QModelIndex parent(const QModelIndex &index) const override
    {
        Node *n = (Node *)index.internalPointer();
        if (!n || n->parent == tree)
            return QModelIndex();
        Q_ASSERT(n->parent->parent);
        int parentRow = n->parent->parent->children.indexOf(n->parent);
        Q_ASSERT(parentRow != -1);
        return createIndex(parentRow, 0, n->parent);
    }

    QVariant data(const QModelIndex &idx, int role) const override
    {
        if (!idx.isValid())
            return QVariant();

        Node *pn = (Node *)idx.internalPointer();
        if (!pn)
            pn = tree;
        if (pn != tree)
            pn = pn->parent;
        if (idx.row() < 0 || idx.column() < 0 || idx.column() >= cols
            || idx.row() >= pn->children.size()) {
            wrongIndex = true;
            qWarning("Invalid modelIndex [%d,%d,%p]", idx.row(), idx.column(),
                     idx.internalPointer());
            return QVariant();
        }

        if (role == Qt::DisplayRole) {
            return displayData(idx);
        }

        return QVariant();
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        Q_UNUSED(value);
        QVector<int> changedRole(1, role);
        emit dataChanged(index, index, changedRole);
        return true;
    }

    void groupedSetData(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
    {
        emit dataChanged(topLeft, bottomRight, roles);
    }

    void changeLayout(const QList<QPersistentModelIndex> &parents = QList<QPersistentModelIndex>())
    {
        emit layoutAboutToBeChanged(parents);
        emit layoutChanged(parents);
    }

    bool removeRows(int row, int count, const QModelIndex &parent) override
    {
        beginRemoveRows(parent, row, row + count - 1);
        Node *n = (Node *)parent.internalPointer();
        if (!n)
            n = tree;
        n->removeRows(row, count);
        endRemoveRows();
        return true;
    }

    void removeLastColumn()
    {
        beginRemoveColumns(QModelIndex(), cols - 1, cols - 1);
        --cols;
        endRemoveColumns();
    }

    void removeAllColumns()
    {
        beginRemoveColumns(QModelIndex(), 0, cols - 1);
        cols = 0;
        endRemoveColumns();
    }

    bool insertRows(int row, int count, const QModelIndex &parent) override
    {
        beginInsertRows(parent, row, row + count - 1);
        Node *n = (Node *)parent.internalPointer();
        if (!n)
            n = tree;
        n->addRows(row, count);
        endInsertRows();
        return true;
    }

    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) override
    {
        Q_ASSERT_X(sourceRow >= 0 && sourceRow < rowCount(sourceParent)
                   && count > 0 && sourceRow + count < rowCount(sourceParent)
                   && destinationChild >= 0 && destinationChild <= rowCount(destinationParent),
                   Q_FUNC_INFO, "Rows out of range.");
        Q_ASSERT_X(!(sourceParent == destinationParent && destinationChild >= sourceRow && destinationChild < sourceRow + count),
                   Q_FUNC_INFO, "Moving rows onto themselves.");
        if (!beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild))
            return false;
        Node *src = (Node *)sourceParent.internalPointer();
        if (!src)
            src = tree;
        Node *dest = (Node *)destinationParent.internalPointer();
        if (!dest)
            dest = tree;
        QVector<Node *> buffer = src->children.mid(sourceRow, count);
        if (src != dest) {
            src->removeRows(sourceRow, count, true /* keep alive */);
            dest->addRows(destinationChild, count);
        } else {
            QVector<Node *> &c = dest->children;
            if (sourceRow < destinationChild) {
                memmove(&c[sourceRow], &c[sourceRow + count], sizeof(Node *) * (destinationChild - sourceRow - count));
                destinationChild -= count;
            } else {
                memmove(&c[destinationChild + count], &c[destinationChild], sizeof(Node *) * (sourceRow - destinationChild));
            }
        }
        for (int i = 0; i < count; i++) {
            Node *n = buffer[i];
            n->parent = dest;
            dest->children[i + destinationChild] = n;
        }

        endMoveRows();
        return true;
    }

    void setDecorationsEnabled(bool enable)
    {
        decorationsEnabled = enable;
    }

    mutable bool fetched;
    bool decorationsEnabled;
    bool alternateChildlessRows;
    int rows, cols;
    int levels;
    mutable bool wrongIndex;

    struct Node {
        Q_DISABLE_COPY_MOVE(Node)

        Node *parent;
        QVector<Node *> children;

        Node(int rows, Node *p = 0) : parent(p)
        {
            addRows(0, rows);
        }

        ~Node()
        {
            qDeleteAll(children);
        }

        void addRows(int row, int count)
        {
            if (count > 0) {
                children.reserve(children.size() + count);
                children.insert(row, count, (Node *)0);
            }
        }

        void removeRows(int row, int count, bool keepAlive = false)
        {
            int newCount = qMax(children.size() - count, 0);
            int effectiveCountDiff = children.size() - newCount;
            if (effectiveCountDiff > 0) {
                if (!keepAlive)
                    for (int i = 0; i < effectiveCountDiff; i++)
                        delete children[i + row];
                children.remove(row, effectiveCountDiff);
            }
        }
    };

    Node *tree;
};

#endif // Q_TEST_MODEL_H
