// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "typerolemodel.h"

TypeRoleModel::TypeRoleModel(QObject *parent)
    : QAbstractListModel(parent)
{
    _mapRoleNames[TypeRole] = "type";
    _mapRoleNames[TextRole] = "text";
}

int TypeRoleModel::rowCount(const QModelIndex &) const
{
    return 3;
}

QVariant TypeRoleModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    constexpr Type types[] = {
        Type::PlainText,
        Type::Markdown,
        Type::Rect
    };
    switch (role) {
    case TypeRole: {
        const Type type = types[index.row() % std::size(types)];
        return QVariant::fromValue(type);
    }
    case TextRole: {
        if (index.row() % std::size(types) == int(Type::Markdown))
            return "*row* " + QString::number(index.row());
        return "row " + QString::number(index.row());
    }
    }

    return {};
}
