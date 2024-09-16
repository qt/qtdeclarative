// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "errorlistmodel.h"

#include <QQmlError>

ErrorListModel::ErrorListModel(QObject *parent)
    : QAbstractListModel{parent}
{}

bool ErrorListModel::isIndexValid(const QModelIndex &index) const
{
    return index.isValid() && (0 <= index.row()) && (index.row() < rowCount());
}

void ErrorListModel::setErrorList(const QList<QQmlError> &errorList)
{
    beginResetModel();
    m_errorList = errorList;
    endResetModel();
}

void ErrorListModel::selectIndex(const QModelIndex &index)
{
    if (!isIndexValid(index))
        return;
    const QQmlError &error = m_errorList.at(index.row());
    emit errorPositionSelected(error.line(), error.column());
}

int ErrorListModel::rowCount(const QModelIndex &parent) const
{
    return m_errorList.size();
}

QVariant ErrorListModel::data(const QModelIndex &index, int role) const
{
    if ((role != Qt::DisplayRole) && (role != Qt::EditRole))
        return QVariant{};

    if (!isIndexValid(index))
        return QVariant{};

    return m_errorList.at(index.row()).toString();
}
