// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "imagemodel.h"

ImageModel::ImageModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QString ImageModel::source() const
{
    return m_source;
}

//! [setsource]
void ImageModel::setSource(const QString &source)
{
    if (m_source == source)
        return;

    beginResetModel();
    m_source = source;
    m_image.load(m_source);
    endResetModel();
}
//! [setsource]

//! [rowcolcount]
int ImageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_image.height();
}

int ImageModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_image.width();
}
//! [rowcolcount]


//! [data]
QVariant ImageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();
    return qGray(m_image.pixel(index.column(), index.row()));
}
//! [data]

QVariant ImageModel::headerData(int /* section */, Qt::Orientation /* orientation */,
                                int role) const
{
    if (role == Qt::SizeHintRole)
        return QSize(1, 1);
    return QVariant();
}
