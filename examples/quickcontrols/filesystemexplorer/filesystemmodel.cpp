// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "filesystemmodel.h"

#include <QMimeDatabase>
#include <QStandardPaths>

FileSystemModel::FileSystemModel(QObject *parent) : QFileSystemModel(parent)
{
    setRootPath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
}

int FileSystemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QString FileSystemModel::readFile(const QString &filePath)
{
    // Don't issue errors for an empty path, as the initial binding
    // will result in an empty path, and that's OK.
    if (filePath.isEmpty())
        return {};

    QFile file(filePath);

    const QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(QFileInfo(file));

    if (file.size() >= 2'000'000)
        return tr("File size is too big.\nYou can read files up to %1 MB.").arg(2);

    // Check if the mimetype is supported and return the content.
    const auto mimeTypesForFile = mime.parentMimeTypes();
    for (const auto &m : mimeTypesForFile) {
        if (m.contains("text", Qt::CaseInsensitive)
                || mime.comment().contains("text", Qt::CaseInsensitive)) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                return tr("Error opening the File!");

            QTextStream stream(&file);
            return stream.readAll();
        }
    }
    return tr("Filetype not supported!");
}
