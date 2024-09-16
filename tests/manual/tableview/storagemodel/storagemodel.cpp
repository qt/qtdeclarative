// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Ivan Komissarov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "storagemodel.h"

#include <QDir>
#include <QLocale>
#include <qmath.h>
#include <algorithm>
#include <cmath>

StorageModel::StorageModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    refresh();
}

QHash<int, QByteArray> StorageModel::roleNames() const {
    static auto roles = [this]() {
        auto roles = QHash<int, QByteArray> {
            { int(Role::Type), "type" },
            { int(Role::Heading), "heading" },
            { int(Role::Value), "value" },
            { int(Role::ValueMax), "valueMax" },
            { int(Role::ValueDisplay), "valueDisplay" },
            { int(Role::ValueMaxDisplay), "valueMaxDisplay" },
            { Qt::CheckStateRole, "checkState" }
        };
        roles.insert(QAbstractTableModel::roleNames());
        return roles;
    }();
    return roles;
}

void StorageModel::refresh()
{
    beginResetModel();
    m_volumes = QStorageInfo::mountedVolumes();
    std::sort(m_volumes.begin(), m_volumes.end(),
              [](const QStorageInfo &st1, const QStorageInfo &st2) {
                  static const QString rootSortString = QStringLiteral(" ");
                  return (st1.isRoot() ? rootSortString : st1.rootPath())
                       < (st2.isRoot() ? rootSortString : st2.rootPath());
              });
    endResetModel();
}

int StorageModel::columnCount(const QModelIndex &/*parent*/) const
{
    return int(Column::Count);
}

int StorageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_volumes.count();
}

Qt::ItemFlags StorageModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags result = QAbstractTableModel::flags(index);
    switch (Column(index.column())) {
    case Column::IsReady:
    case Column::IsReadOnly:
    case Column::IsValid:
        result |= Qt::ItemIsUserCheckable;
        break;
    default:
        break;
    }
    return result;
}

QVariant StorageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
    case int(Role::ValueDisplay): {
        const QStorageInfo &volume = m_volumes.at(index.row());
        switch (Column(index.column())) {
        case Column::RootPath:
            return QDir::toNativeSeparators(volume.rootPath());
        case Column::Name:
            return volume.name();
        case Column::Device:
            return volume.device();
        case Column::FileSystemName:
            return volume.fileSystemType();
        case Column::Free:
            return QLocale().formattedDataSize(volume.bytesFree());
        case Column::IsReady:
            return volume.isReady();
        case Column::IsReadOnly:
            return volume.isReadOnly();
        case Column::IsValid:
            return volume.isValid();
        default:
            break;
        }
    } break;
    case Qt::CheckStateRole: {
        const QStorageInfo &volume = m_volumes.at(index.row());
        switch (Column(index.column())) {
        case Column::IsReady:
            return volume.isReady();
        case Column::IsReadOnly:
            return volume.isReadOnly();
        case Column::IsValid:
            return volume.isValid();
        default:
            break;
        }
    } break;
    case Qt::TextAlignmentRole:
        switch (Column(index.column())) {
        case Column::Free:
            return Qt::AlignTrailing;
        default:
            break;
        }
        return Qt::AlignLeading;
    case Qt::ToolTipRole: {
        QLocale locale;
        const QStorageInfo &volume = m_volumes.at(index.row());
        return tr("Root path : %1\n"
                  "Name: %2\n"
                  "Display Name: %3\n"
                  "Device: %4\n"
                  "FileSystem: %5\n"
                  "Total size: %6\n"
                  "Free size: %7\n"
                  "Available size: %8\n"
                  "Is Ready: %9\n"
                  "Is Read-only: %10\n"
                  "Is Valid: %11\n"
                  "Is Root: %12"
                  ).
                arg(QDir::toNativeSeparators(volume.rootPath())).
                arg(volume.name()).
                arg(volume.displayName()).
                arg(QString::fromUtf8(volume.device())).
                arg(QString::fromUtf8(volume.fileSystemType())).
                arg(locale.formattedDataSize(volume.bytesTotal())).
                arg(locale.formattedDataSize(volume.bytesFree())).
                arg(locale.formattedDataSize(volume.bytesAvailable())).
                arg(volume.isReady() ? tr("true") : tr("false")).
                arg(volume.isReadOnly() ? tr("true") : tr("false")).
                arg(volume.isValid() ? tr("true") : tr("false")).
                arg(volume.isRoot() ? tr("true") : tr("false"));
    }
    case int(Role::Type):
        switch (Column(index.column())) {
        case Column::RootPath:
        case Column::Name:
        case Column::Device:
        case Column::FileSystemName:
            return QVariant::fromValue(Type::String);
            break;
        case Column::Free:
            return QVariant::fromValue(Type::Value);
        case Column::IsReady:
        case Column::IsReadOnly:
        case Column::IsValid:
            return QVariant::fromValue(Type::Flag);
        default:
            break;
        }
        break;
    case int(Role::Heading):
        return headerData(index.column());
    case int(Role::Value):
        switch (Column(index.column())) {
        case Column::Free:
            return m_volumes.at(index.row()).bytesFree();
        default:
            break;
        }
        break;
    case int(Role::ValueMax):
        switch (Column(index.column())) {
        case Column::Free:
            return m_volumes.at(index.row()).bytesTotal();
        default:
            break;
        }
        break;
    case int(Role::ValueMaxDisplay):
        switch (Column(index.column())) {
        case Column::Free:
            return QLocale().formattedDataSize(m_volumes.at(index.row()).bytesTotal());
        default:
            break;
        }
        break;
    } // switch (role)
    return QVariant();
}

QVariant StorageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    switch (Column(section)) {
    case Column::RootPath:
        return tr("Root Path");
    case Column::Name:
        return tr("Volume Name");
    case Column::Device:
        return tr("Device");
    case Column::FileSystemName:
        return tr("File System");
    case Column::Free:
        return tr("Free");
    case Column::IsReady:
        return tr("Ready");
    case Column::IsReadOnly:
        return tr("Read-only");
    case Column::IsValid:
        return tr("Valid");
    default:
        break;
    }

    return QVariant();
}
