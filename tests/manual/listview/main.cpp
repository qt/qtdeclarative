// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QAbstractTableModel>

typedef QPair<QString, bool> CellData;

class TestModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int rowCount READ rowCount WRITE setRowCount NOTIFY rowCountChanged)

public:
    TestModel(QObject *parent = nullptr) : QAbstractListModel(parent) { }

    int rowCount(const QModelIndex & = QModelIndex()) const override { return m_rows; }
    void setRowCount(int count) {
        m_rows = count;
        emit rowCountChanged();
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid())
            return QVariant();

        switch (role) {
        case Qt::DisplayRole:
            return index.row() % 2 ? QStringLiteral("type2") : QStringLiteral("type1");
        default:
            break;
        }

        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {Qt::DisplayRole, "delegateType"},
        };
    }

signals:
    void rowCountChanged();

private:
    int m_rows = 0;
};

#include "main.moc"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<TestModel>("TestModel", 0, 1, "TestModel");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}
