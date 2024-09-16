// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "gadgetmodel.h"

GadgetModel::GadgetModel(QObject* parent)
    : QAbstractListModel(parent)
      , m_gadgets{ { "hamster", "small" },
                 { "mouse", "small" },
                 { "lion", "medium" },
                 { "elephant", "large" } }
{
}

int GadgetModel::rowCount(const QModelIndex& parent) const
{
    return m_gadgets.size();
}

QVariant GadgetModel::data(const QModelIndex& index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid))
        return {};

    return QVariant::fromValue(m_gadgets[index.row()]);
}

QHash<int, QByteArray> GadgetModel::roleNames() const
{
    static const QHash<int, QByteArray> h = { { 0, "gadget" } };
    return h;
}

MyGadget::MyGadget(QString name, QString size)
    : m_name(name)
      , m_size(size)
{
}
