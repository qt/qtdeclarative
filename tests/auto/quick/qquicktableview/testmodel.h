// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QtCore>
#include <QtGui/QStandardItemModel>

class TestModel : public QAbstractTableModel
{
    Q_OBJECT
    Q_PROPERTY(int rowCount READ rowCount WRITE setRowCount NOTIFY rowCountChanged)
    Q_PROPERTY(int columnCount READ columnCount WRITE setColumnCount NOTIFY columnCountChanged)

public:
    TestModel(QObject *parent = nullptr)
        : QAbstractTableModel(parent)
    {}

    TestModel(int rows, int columns, QObject *parent = nullptr)
        : QAbstractTableModel(parent)
        , m_rows(rows)
        , m_columns(columns)
    {}

    TestModel(int rows, int columns, bool dataCanBeFetched, QObject *parent = nullptr)
        : QAbstractTableModel(parent)
          , m_rows(rows)
          , m_columns(columns)
          , m_dataCanBeFetched(dataCanBeFetched)
    {}

    int rowCount(const QModelIndex & = QModelIndex()) const override { return m_rows; }
    void setRowCount(int count) { beginResetModel(); m_rows = count; emit rowCountChanged(); endResetModel(); }

    int columnCount(const QModelIndex & = QModelIndex()) const override { return m_columns; }
    void setColumnCount(int count) { beginResetModel(); m_columns = count; emit columnCountChanged(); endResetModel(); }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid())
            return QVariant();

        QVariant ret;

        switch (role) {
        case Qt::UserRole:
            ret = 42;
            break;
        case Qt::DisplayRole: {
            const QPoint cell(index.column(), index.row());
            if (modelData.contains(cell))
                ret = modelData.value(cell);
            else
                ret = QStringLiteral("%1").arg(index.row()); }
            break;
        default:
            break;
        }

        return ret;
    }

    Q_INVOKABLE QVariant dataFromFlatIndex(int flatIndex) const
    {
        const int row = flatIndex % m_rows;
        const int column = flatIndex / m_rows;
        return modelData.value(QPoint(column, row));
    }

    QHash<int, QByteArray> roleNames() const override
    {
        if (m_useCustomRoleNames)
            return { { Qt::UserRole, "custom"} };
        return { {Qt::DisplayRole, "display"} };
    }

    Q_INVOKABLE void useCustomRoleNames(bool use)
    {
        beginResetModel();
        m_useCustomRoleNames = use;
        endResetModel();
    }

    Q_INVOKABLE void setModelData(const QPoint &cell, const QSize &span, const QString &string)
    {
        for (int c = 0; c < span.width(); ++c) {
            for (int r = 0; r < span.height(); ++r) {
                const int changedRow = cell.y() + r;
                const int changedColumn = cell.x() + c;
                modelData.insert(QPoint(changedColumn, changedRow), string);
            }
        }

        const auto topLeftIndex = createIndex(cell.y(), cell.x(), nullptr);
        const auto bottomRightIndex = createIndex(cell.y() + span.height() - 1, cell.x() + span.width() - 1, nullptr);
        emit dataChanged(topLeftIndex, bottomRightIndex);
    }

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override
    {
        if (row < 0 || count <= 0)
            return false;

        beginInsertRows(parent, row, row + count - 1);
        m_rows += count;
        shiftModelDataRows(row, count);
        endInsertRows();
        return true;
    }

    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override
    {
        if (!checkIndex(createIndex(row, 0)) || !checkIndex(createIndex(row + count - 1, 0)))
            return false;

        beginRemoveRows(parent, row, row + count - 1);
        m_rows -= count;
        shiftModelDataRows(row + count, -count);
        endRemoveRows();
        return true;
    }

    bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override
    {
        if (column < 0 || count <= 0)
            return false;

        beginInsertColumns(parent, column, column + count - 1);
        m_columns += count;
        shiftModelDataColumns(column, count);
        endInsertColumns();
        return true;
    }

    bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override
    {
        if (!checkIndex(createIndex(0, column)) || !checkIndex(createIndex(0, column + count - 1)))
            return false;

        beginRemoveColumns(parent, column, column + count - 1);
        m_columns -= count;
        shiftModelDataColumns(column + count, -count);
        endRemoveColumns();
        return true;
    }

    bool canFetchMore(const QModelIndex &parent) const override
    {
        Q_UNUSED(parent);
        return m_dataCanBeFetched;
    }

    void swapRows(int row1, int row2)
    {
        layoutAboutToBeChanged();
        for (int c = 0; c < m_columns; ++c) {
            const QPoint cell1(c, row1);
            const QPoint cell2(c, row2);
            const QString cell1Data = modelData.value(cell1);

            // Do the data swap, but since modelData is not supposed to contain
            // cells with empty strings, we need to check for this.
            if (modelData.contains(cell2))
                modelData[cell1] = modelData[cell2];
            else
                modelData.remove(cell1);

            if (!cell1Data.isEmpty())
                modelData[cell2] = cell1Data;
            else
                modelData.remove(cell2);
        }

        // Since this is a row reorder and not just a data update, we must
        // remap any persistent model indexes pointing into the swapped rows.
        // This is required by the layoutAboutToBeChanged/layoutChanged contract.
        QModelIndexList from, to;
        for (const QModelIndex &idx : persistentIndexList()) {
            if (idx.row() == row1) {
                from << idx;
                to << createIndex(row2, idx.column());
            } else if (idx.row() == row2) {
                from << idx;
                to << createIndex(row1, idx.column());
            }
        }
        changePersistentIndexList(from, to);
        layoutChanged();
    }

    void fetchMore(const QModelIndex &parent) override
    {
        Q_UNUSED(parent);
        addRow(m_rows - 1);
    }

    void clear() {
        beginResetModel();
        m_rows = 0;
        m_columns = 0;
        modelData.clear();
        endResetModel();
    }

    Q_INVOKABLE void addRow(int row)
    {
        insertRow(row, QModelIndex());
    }

    Q_INVOKABLE void addColumn(int column)
    {
        insertColumn(column, QModelIndex());
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        Q_UNUSED(index)
        return m_flags;
    }

    void setFlags(Qt::ItemFlags flags)
    {
        m_flags = flags;
    }

signals:
    void rowCountChanged();
    void columnCountChanged();

private:

    void shiftModelDataRows(int firstRow, int delta)
    {
        // Shift all modelData entries whose row >= firstRow by delta
        // (positive delta = insert, negative delta = remove).
        QHash<QPoint, QString> updated;
        for (auto it = modelData.begin(); it != modelData.end(); ++it) {
            const int row = it.key().y();
            if (delta < 0) {
                const int removedCount = -delta;
                if (row < firstRow && row >= firstRow - removedCount)
                    continue; // row was removed
            }
            const int newRow = row < firstRow ? row : row + delta;
            updated.insert(QPoint(it.key().x(), newRow), it.value());
        }
        modelData = std::move(updated);
    }

    void shiftModelDataColumns(int firstColumn, int delta)
    {
        // Shift all modelData entries whose column >= firstColumn by delta.
        // (positive delta = insert, negative delta = remove).
        QHash<QPoint, QString> updated;
        for (auto it = modelData.begin(); it != modelData.end(); ++it) {
            const int column = it.key().x();
            if (delta < 0) {
                const int removedCount = -delta;
                if (column < firstColumn && column >= firstColumn - removedCount)
                    continue; // column was removed
            }
            const int newColumn = column < firstColumn ? column : column + delta;
            updated.insert(QPoint(newColumn, it.key().y()), it.value());
        }
        modelData = std::move(updated);
    }

private:

    int m_rows = 0;
    int m_columns = 0;
    bool m_dataCanBeFetched = false;
    bool m_useCustomRoleNames = false;
    QHash<QPoint, QString> modelData;
    Qt::ItemFlags m_flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
};

#define TestModelAsVariant(...) QVariant::fromValue(QSharedPointer<TestModel>(new TestModel(__VA_ARGS__)))
