/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
        if (!index.isValid() || role != Qt::DisplayRole)
            return QVariant();

        int serializedIndex = index.row() + (index.column() * m_columns);
        if (modelData.contains(serializedIndex))
            return modelData.value(serializedIndex);
        return QStringLiteral("%1").arg(index.row());
    }

    Q_INVOKABLE QVariant dataFromSerializedIndex(int index) const
    {
        if (modelData.contains(index))
            return modelData.value(index);
        return QString();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return { {Qt::DisplayRole, "display"} };
    }

    Q_INVOKABLE void setModelData(const QPoint &cell, const QSize &span, const QString &string)
    {
        for (int c = 0; c < span.width(); ++c) {
            for (int r = 0; r < span.height(); ++r) {
                const int changedRow = cell.y() + r;
                const int changedColumn = cell.x() + c;
                const int serializedIndex = changedRow + (changedColumn * m_rows);
                modelData.insert(serializedIndex, string);
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
        endInsertRows();
        return true;
    }

    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override
    {
        if (!checkIndex(createIndex(row, 0)) || !checkIndex(createIndex(row + count - 1, 0)))
            return false;

        beginRemoveRows(parent, row, row + count - 1);
        m_rows -= count;
        for (int c = 0; c < m_columns; ++c) {
            for (int r = 0; r < count; ++r) {
                const int serializedIndex = (row + r) + (c * m_rows);
                modelData.remove(serializedIndex);
            }
        }
        endRemoveRows();
        return true;
    }

    bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override
    {
        if (column < 0 || count <= 0)
            return false;

        beginInsertColumns(parent, column, column + count - 1);
        m_columns += count;
        endInsertColumns();
        return true;
    }

    bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override
    {
        if (!checkIndex(createIndex(0, column)) || !checkIndex(createIndex(0, column + count - 1)))
            return false;

        beginRemoveColumns(parent, column, column + count - 1);
        m_columns -= count;
        endRemoveColumns();
        return true;
    }

    bool canFetchMore(const QModelIndex &parent) const override
    {
        Q_UNUSED(parent)
        return m_dataCanBeFetched;
    }

    void swapRows(int row1, int row2)
    {
        layoutAboutToBeChanged();
        Q_ASSERT(modelData.contains(row1));
        Q_ASSERT(modelData.contains(row2));
        const QString tmp = modelData[row1];
        modelData[row1] = modelData[row2];
        modelData[row2] = tmp;
        layoutChanged();
    }

    void fetchMore(const QModelIndex &parent) override
    {
        Q_UNUSED(parent)
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

signals:
    void rowCountChanged();
    void columnCountChanged();

private:
    int m_rows = 0;
    int m_columns = 0;
    bool m_dataCanBeFetched = false;
    QHash<int, QString> modelData;
};

#define TestModelAsVariant(...) QVariant::fromValue(QSharedPointer<TestModel>(new TestModel(__VA_ARGS__)))
