// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "model.h"

Animal::Animal(const QString &type, const QString &size)
    : m_type(type), m_size(size)
{
}

QString Animal::type() const
{
    return m_type;
}

QString Animal::size() const
{
    return m_size;
}

AnimalModel::AnimalModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void AnimalModel::addAnimal(const Animal &animal)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    m_animals << animal;
    endInsertRows();
}

int AnimalModel::rowCount(const QModelIndex & parent) const {
    Q_UNUSED(parent);
    return m_animals.count();
}

QVariant AnimalModel::data(const QModelIndex & index, int role) const {
    if (index.row() < 0 || index.row() >= m_animals.count())
        return QVariant();

    const Animal &animal = m_animals[index.row()];
    if (role == TypeRole)
        return animal.type();
    else if (role == SizeRole)
        return animal.size();
    return QVariant();
}

//![0]
QHash<int, QByteArray> AnimalModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[TypeRole] = "type";
    roles[SizeRole] = "size";
    return roles;
}
//![0]

