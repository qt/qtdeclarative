// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef GAMEOFLIFEMODEL_H
#define GAMEOFLIFEMODEL_H

#include <array>
#include <QAbstractTableModel>
#include <QPoint>
#include <QtQml/qqml.h>

//! [modelclass]
class GameOfLifeModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        CellRole
    };
    Q_ENUM(Roles)

    QHash<int, QByteArray> roleNames() const override {
        return {
            { CellRole, "value" }
        };
    }

    explicit GameOfLifeModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    Q_INVOKABLE void nextStep();
    Q_INVOKABLE bool loadFile(const QString &fileName);
    Q_INVOKABLE void loadPattern(const QString &plainText);
    Q_INVOKABLE void clear();

private:
    static constexpr int width = 256;
    static constexpr int height = 256;
    static constexpr int size = width * height;

    using StateContainer = std::array<bool, size>;
    StateContainer m_currentState;

    int cellNeighborsCount(const QPoint &cellCoordinates) const;
    static bool areCellCoordinatesValid(const QPoint &coordinates);
    static QPoint cellCoordinatesFromIndex(int cellIndex);
    static std::size_t cellIndex(const QPoint &coordinates);
};
//! [modelclass]

#endif // GAMEOFLIFEMODEL_H
