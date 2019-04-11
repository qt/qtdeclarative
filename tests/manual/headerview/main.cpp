/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QAbstractTableModel>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

class TestTableModel : public QAbstractTableModel {
    Q_OBJECT
    Q_PROPERTY(int rowCount READ rowCount WRITE setRowCount NOTIFY rowCountChanged)
    Q_PROPERTY(int columnCount READ columnCount WRITE setColumnCount NOTIFY columnCountChanged)

public:
    TestTableModel(QObject *parent = nullptr)
        : QAbstractTableModel(parent)
    {
    }

    int rowCount(const QModelIndex & = QModelIndex()) const override
    {
        return m_rows;
    }
    virtual void setRowCount(int count)
    {
        beginResetModel();
        m_rows = count;
        emit rowCountChanged();
        endResetModel();
    }

    int columnCount(const QModelIndex & = QModelIndex()) const override
    {
        return m_cols;
    }
    virtual void setColumnCount(int count)
    {
        beginResetModel();
        m_cols = count;
        emit columnCountChanged();
        endResetModel();
    }

    int indexValue(const QModelIndex &index) const
    {
        return index.row() + (index.column() * rowCount());
    }

    Q_INVOKABLE QModelIndex toQModelIndex(int serialIndex)
    {
        return createIndex(serialIndex % rowCount(), serialIndex / rowCount());
    }

    Q_INVOKABLE QVariant data(int row, int col)
    {
        return data(createIndex(row, col), Qt::DisplayRole);
    }
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid())
            return QVariant();

        switch (role) {
        case Qt::DisplayRole:
            return QLatin1String("Foo");
        case Qt::EditRole:
            return m_checkedCells.contains(indexValue(index));
        default:
            return QVariant();
        }
    }

    bool setData(const QModelIndex &index, const QVariant &value,
        int role = Qt::EditRole) override
    {

        if (role != Qt::EditRole)
            return false;

        int i = indexValue(index);
        bool checked = value.toBool();
        if (checked == m_checkedCells.contains(i))
            return false;

        if (checked)
            m_checkedCells.insert(i);
        else
            m_checkedCells.remove(i);

        emit dataChanged(index, index, { role });
        return true;
    }

    Q_INVOKABLE QHash<int, QByteArray> roleNames() const override
    {
        return {
            { Qt::DisplayRole, "display" },
            { Qt::EditRole, "edit" }
        };
    }

signals:
    void rowCountChanged();
    void columnCountChanged();

private:
    int m_rows = 0;
    int m_cols = 0;

    QSet<int> m_checkedCells;
};

class TestTableModelWithHeader : public TestTableModel {

    Q_OBJECT
public:
    void setRowCount(int count) override
    {
        vData.resize(count);
        TestTableModel::setRowCount(count);
    }

    void setColumnCount(int count) override
    {
        hData.resize(count);
        TestTableModel::setColumnCount(count);
    }

    Q_INVOKABLE QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override
    {
        const bool isHorizontal = orientation == Qt::Horizontal;
        auto sectionCount = isHorizontal ? columnCount() : rowCount();
        if (section < 0 || section >= sectionCount)
            return QVariant();
        switch (role) {
        case Qt::DisplayRole:
            return (isHorizontal ? QString::fromLatin1("Column %1") : QString::fromLatin1("Row %1")).arg(section);
        case Qt::EditRole: {
            auto &data = isHorizontal ? hData : vData;
            return data[section].toString();
        }
        default:
            return QVariant();
        }
    }

    Q_INVOKABLE bool setHeaderData(int section, Qt::Orientation orientation,
        const QVariant &value, int role = Qt::EditRole) override
    {
        qDebug() << Q_FUNC_INFO
                 << "section:" << section
                 << "orient:" << orientation
                 << "value:" << value
                 << "role:" << QAbstractItemModel::roleNames()[role];
        auto sectionCount = orientation == Qt::Horizontal ? columnCount() : rowCount();
        if (section < 0 || section >= sectionCount)
            return false;
        auto &data = orientation == Qt::Horizontal ? hData : vData;
        data[section] = value;
        emit headerDataChanged(orientation, section, section);
        return true;
    }

private:
    QVector<QVariant> hData, vData;
};

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    qmlRegisterType<TestTableModel>("TestTableModel", 0, 1, "TestTableModel");
    qmlRegisterType<TestTableModelWithHeader>("TestTableModelWithHeader", 0, 1, "TestTableModelWithHeader");
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}

#include "main.moc"
