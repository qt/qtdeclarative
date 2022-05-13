// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "gameoflifemodel.h"
#include <QFile>
#include <QTextStream>
#include <QRect>

GameOfLifeModel::GameOfLifeModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    clear();
}

//! [modelsize]
int GameOfLifeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return height;
}

int GameOfLifeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return width;
}
//! [modelsize]

//! [read]
QVariant GameOfLifeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != CellRole)
        return QVariant();

    return QVariant(m_currentState[cellIndex({index.column(), index.row()})]);
}
//! [read]

//! [write]
bool GameOfLifeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != CellRole || data(index, role) == value)
        return false;

    m_currentState[cellIndex({index.column(), index.row()})] = value.toBool();
    emit dataChanged(index, index, {role});

    return true;
}
//! [write]

Qt::ItemFlags GameOfLifeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEditable;
}

//! [update]
void GameOfLifeModel::nextStep()
{
    StateContainer newValues;

    for (std::size_t i = 0; i < size; ++i) {
        bool currentState = m_currentState[i];

        int cellNeighborsCount = this->cellNeighborsCount(cellCoordinatesFromIndex(static_cast<int>(i)));

        newValues[i] = currentState == true
                ? cellNeighborsCount == 2 || cellNeighborsCount == 3
                : cellNeighborsCount == 3;
    }

    m_currentState = std::move(newValues);

    emit dataChanged(index(0, 0), index(height - 1, width - 1), {CellRole});
}
//! [update]

//! [loader]
bool GameOfLifeModel::loadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QTextStream in(&file);
    loadPattern(in.readAll());

    return true;
}

void GameOfLifeModel::loadPattern(const QString &plainText)
{
    clear();

    QStringList rows = plainText.split("\n");
    QSize patternSize(0, rows.count());
    for (QString row : rows) {
        if (row.size() > patternSize.width())
            patternSize.setWidth(row.size());
    }

    QPoint patternLocation((width - patternSize.width()) / 2, (height - patternSize.height()) / 2);

    for (int y = 0; y < patternSize.height(); ++y) {
        const QString line = rows[y];

        for (int x = 0; x < line.length(); ++x) {
            QPoint cellPosition(x + patternLocation.x(), y + patternLocation.y());
            m_currentState[cellIndex(cellPosition)] = line[x] == 'O';
        }
    }

    emit dataChanged(index(0, 0), index(height - 1, width - 1), {CellRole});
}
//! [loader]

void GameOfLifeModel::clear()
{
    m_currentState.fill(false);
    emit dataChanged(index(0, 0), index(height - 1, width - 1), {CellRole});
}

int GameOfLifeModel::cellNeighborsCount(const QPoint &cellCoordinates) const
{
    int count = 0;

    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            if (x == 0 && y == 0)
                continue;

            const QPoint neighborPosition { cellCoordinates.x() + x, cellCoordinates.y() + y };
            if (!areCellCoordinatesValid(neighborPosition))
                continue;

            if (m_currentState[cellIndex(neighborPosition)])
                ++count;

            if (count > 3)
                return count;
        }
    }

    return count;
}

bool GameOfLifeModel::areCellCoordinatesValid(const QPoint &coordinates)
{
    return QRect(0, 0, width, height).contains(coordinates);
}

QPoint GameOfLifeModel::cellCoordinatesFromIndex(int cellIndex)
{
    return {cellIndex % width, cellIndex / width};
}

std::size_t GameOfLifeModel::cellIndex(const QPoint &coordinates)
{
    return std::size_t(coordinates.y() * width + coordinates.x());
}
