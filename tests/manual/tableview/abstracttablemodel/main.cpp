// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QAbstractTableModel>
#include <QSet>
#include <QDebug>

typedef QPair<QString, bool> CellData;

class TestTableModel : public QAbstractTableModel
{
    Q_OBJECT
    Q_PROPERTY(int rowCount READ rowCount WRITE setRowCount NOTIFY rowCountChanged)
    Q_PROPERTY(int columnCount READ columnCount WRITE setColumnCount NOTIFY columnCountChanged)

public:
    TestTableModel(QObject *parent = nullptr) : QAbstractTableModel(parent) { }

    int rowCount(const QModelIndex & = QModelIndex()) const override { return m_rows; }
    void setRowCount(int count) {
        beginResetModel();
        m_rows = count;
        rebuildModel();
        emit rowCountChanged();
        endResetModel();
    }

    int columnCount(const QModelIndex & = QModelIndex()) const override { return m_cols; }
    void setColumnCount(int count) {
        beginResetModel();
        m_cols = count;
        rebuildModel();
        emit columnCountChanged();
        endResetModel();
    }

    int indexValue(const QModelIndex &index) const
    {
        return index.row() + (index.column() * rowCount());
    }

    void rebuildModel()
    {
        m_modelData = QVector<QVector<CellData>>(m_cols);
        for (int x = 0; x < m_cols; ++x) {
            m_modelData[x] = QVector<CellData>(m_rows);
            for (int y = 0; y < m_rows; ++y)
                m_modelData[x][y] = qMakePair(QString("%1, %2").arg(x).arg(y), false);
        }
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid())
            return QVariant();

        switch (role) {
        case Qt::DisplayRole:
            return m_modelData[index.column()][index.row()].first;
        case Qt::CheckStateRole:
            return m_modelData[index.column()][index.row()].second;
        default:
            return QVariant();
        }

        return QVariant();
    }

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override
    {
        switch (role) {
        case Qt::DisplayRole: {
            QString text = value.toString();
            auto &cellData = m_modelData[index.column()][index.row()];
            if (text == cellData.first)
                return false;
            cellData.first = text;
            break; }
        case Qt::CheckStateRole: {
            bool checked = value.toBool();
            auto &cellData = m_modelData[index.column()][index.row()];
            if (checked == cellData.second)
                return false;

            cellData.second = checked;
            break; }
        default:
            return QAbstractTableModel::setData(index, value, role);
        }

        emit dataChanged(index, index, {role});
        return true;
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {Qt::DisplayRole, "display"},
            {Qt::CheckStateRole, "checked"}
        };
    }

    Q_INVOKABLE void insertRows(int row, int count)
    {
        insertRows(row, count, QModelIndex());
    }

    Q_INVOKABLE void removeRows(int row, int count)
    {
        removeRows(row, count, QModelIndex());
    }

    Q_INVOKABLE void insertColumns(int column, int count)
    {
        insertColumns(column, count, QModelIndex());
    }

    Q_INVOKABLE void removeColumns(int column, int count)
    {
        removeColumns(column, count, QModelIndex());
    }

    bool insertRows(int row, int count, const QModelIndex &) override
    {
        if (row > m_rows)
            return false;

        beginInsertRows(QModelIndex(), row, row + count - 1);

        m_rows += count;

        for (int y = 0; y < count; ++y) {
            for (int x = 0; x < m_cols; ++x)
                m_modelData[x].insert(row, qMakePair(QStringLiteral("added"), false));
        }

        endInsertRows();

        return true;
    }

    bool removeRows(int row, int count, const QModelIndex &) override
    {
        if (row + count > m_rows)
            count = m_rows - row;
        if (count < 1)
            return false;

        beginRemoveRows(QModelIndex(), row, row + count - 1);

        m_rows -= count;

        for (int y = 0; y < count; ++y) {
            for (int x = 0; x < m_cols; ++x)
                m_modelData[x].remove(row);
        }

        endRemoveRows();

        return true;
    }

    bool insertColumns(int column, int count, const QModelIndex &) override
    {
        if (column > m_cols)
            return false;

        beginInsertColumns(QModelIndex(), column, column + count - 1);

        m_cols += count;

        for (int x = 0; x < count; ++x) {
            const int c = column + x;
            m_modelData.insert(c, QVector<CellData>(m_rows));
            for (int y = 0; y < m_rows; ++y)
                m_modelData[c][y] = qMakePair(QStringLiteral("added"), false);
        }

        endInsertColumns();

        return true;
    }

    bool removeColumns(int column, int count, const QModelIndex &) override
    {
        if (column + count > m_cols)
            count = m_cols - column;
        if (count < 1)
            return false;

        beginRemoveColumns(QModelIndex(), column, column + count - 1);

        m_cols -= count;

        for (int x = 0; x < count; ++x)
            m_modelData.remove(column + x);

        endRemoveColumns();

        return true;
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        Q_UNUSED(index)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    }

signals:
    void rowCountChanged();
    void columnCountChanged();

private:
    int m_rows = 0;
    int m_cols = 0;

    QVector<QVector<CellData>> m_modelData;
};

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<TestTableModel>("TestTableModel", 0, 1, "TestTableModel");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}

#include "main.moc"
